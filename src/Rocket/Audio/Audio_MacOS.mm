/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <unistd.h>
#import <AVFoundation/AVFoundation.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Audio/Audio.hpp>

/** Completion marker shared with the player's completion handler block. */
@interface __AudioFlag : NSObject {
    @public volatile bool finished;
}
@end

@implementation __AudioFlag
@end

namespace Rocket {

struct Audio::_Audio {
    AVAudioPCMBuffer* buffer = nil;
};

struct Sound::_Sound {
    AVAudioPlayerNode* node = nil;
    AVAudioUnitVarispeed* varispeed = nil;
    AVAudioPCMBuffer* segment = nil;
    AVAudioPCMBuffer* shaped = nil;
    AVAudioPCMBuffer* remainder = nil;
    __AudioFlag* flag = nil;
    float position = 0.0f;
    float offset = 0.0f;

    AVAudioPCMBuffer* activeSegment() const;
    float segmentLength() const;
    void releaseExtras();
    void scheduleFrom(bool loop, float from);
};

static AVAudioEngine* _engine = nil;

static AVAudioEngine* _Engine() {
    PROFILE

    if (_engine == nil) {
        _engine = [[AVAudioEngine alloc] init];
    }

    return _engine;
}

static void _StartEngine() {
    PROFILE

    auto engine = _Engine();

    if (engine.running == NO) {
        NSError* error = nil;

        if ([engine startAndReturnError: &error] == NO) {
            throw std::runtime_error([[error localizedDescription] UTF8String]);
        }
    }
}

AVAudioPCMBuffer* Sound::_Sound::activeSegment() const {
    PROFILE

    return ((shaped != nil) ? shaped : segment);
}

float Sound::_Sound::segmentLength() const {
    PROFILE

    auto buffer = activeSegment();

    return ((float)buffer.frameLength / (float)buffer.format.sampleRate);
}

void Sound::_Sound::releaseExtras() {
    PROFILE

    flag = nil;      /* ARC releases */
    remainder = nil; /* ARC releases */
}

void Sound::_Sound::scheduleFrom(bool loop, float from) {
    PROFILE

    auto buffer = activeSegment();
    auto const rate = (float)buffer.format.sampleRate;
    auto startFrame = (AVAudioFrameCount)(from * rate);

    if (startFrame >= buffer.frameLength) {
        startFrame = 0;
        from = 0.0f;
    }

    [node stop];

    releaseExtras();

    offset = from;

    auto head = buffer;

    if (startFrame > 0) {
        auto const frameCount = (buffer.frameLength - startFrame);
        auto tail = [[AVAudioPCMBuffer alloc]
            initWithPCMFormat: buffer.format
                frameCapacity: frameCount];

        assert(tail != nil);

        for (AVAudioChannelCount channel = 0; channel < buffer.format.channelCount; channel++) {
            std::memcpy(
                tail.floatChannelData[channel],
                (buffer.floatChannelData[channel] + startFrame),
                ((std::size_t)frameCount * sizeof(float)));
        }

        tail.frameLength = frameCount;

        remainder = tail;
        head = tail;
    }

    if (loop) {
        if (head != buffer) {
            [node scheduleBuffer: head atTime: nil options: 0 completionHandler: nil];
        }

        [node scheduleBuffer: buffer
                      atTime: nil
                     options: AVAudioPlayerNodeBufferLoops
           completionHandler: nil];
    } else {
        auto done = [[__AudioFlag alloc] init];

        flag = done;

        [node scheduleBuffer: head
                      atTime: nil
                     options: 0
      completionCallbackType: AVAudioPlayerNodeCompletionDataPlayedBack
           completionHandler: ^(AVAudioPlayerNodeCompletionCallbackType type) {
               (void)type;
               done->finished = true;
           }];
    }
}

void Audio::__init(std::string const& path) {
    PROFILE

    auto url = [NSURL fileURLWithPath: [NSString stringWithUTF8String: path.c_str()]];
    NSError* error = nil;
    auto file = [[AVAudioFile alloc] initForReading: url error: &error];

    if (file == nil) {
        throw std::runtime_error("Failed to load audio: " + path);
    }

    auto buffer = [[AVAudioPCMBuffer alloc]
        initWithPCMFormat: file.processingFormat
            frameCapacity: (AVAudioFrameCount)file.length];

    assert(buffer != nil);

    if ([file readIntoBuffer: buffer error: &error] == NO) {
        throw std::runtime_error("Failed to decode audio: " + path);
    }

    _impl = new _Audio{};
    _impl->buffer = buffer;
}

void Audio::__init(std::uint8_t const* data, std::size_t size) {
    PROFILE

    // AVAudioFile has no in-memory API, so the bytes take a round trip
    // through a temporary file.
    auto path = [NSTemporaryDirectory() stringByAppendingPathComponent:
        [NSString stringWithFormat: @"rocket-audio-%d-%p.bin", getpid(), (void const*)data]];
    auto contents = [NSData dataWithBytes: data length: size];

    if ([contents writeToFile: path atomically: YES] == NO) {
        throw std::runtime_error("Failed to stage audio data");
    }

    try {
        __init(std::string([path fileSystemRepresentation]));
    } catch (...) {
        [[NSFileManager defaultManager] removeItemAtPath: path error: nil];
        throw;
    }

    [[NSFileManager defaultManager] removeItemAtPath: path error: nil];
}

void Audio::__done() {
    PROFILE

    if (_impl != nullptr) {
        delete _impl; // ARC releases the buffer
        _impl = nullptr;
    }
}

float Audio::__getDuration() const {
    PROFILE

    auto buffer = _impl->buffer;

    return ((float)buffer.frameLength / (float)buffer.format.sampleRate);
}

float Audio::__getSampleRate() const {
    PROFILE

    return (float)_impl->buffer.format.sampleRate;
}

std::int32_t Audio::__getChannelCount() const {
    PROFILE

    return (std::int32_t)_impl->buffer.format.channelCount;
}

std::int64_t Audio::__getFrameCount() const {
    PROFILE

    return (std::int64_t)_impl->buffer.frameLength;
}

float const* Audio::__getSamples(std::int32_t channel) const {
    PROFILE

    /* AVAudioFile's processingFormat is non-interleaved float32, so each
       channel is one contiguous array. */
    return _impl->buffer.floatChannelData[channel];
}

void Sound::__init(float start, float length) {
    PROFILE

    auto source = _audio->_impl->buffer;
    auto const rate = (float)source.format.sampleRate;
    auto const startFrame = (AVAudioFrameCount)(start * rate);
    auto const frameCount = std::min(
        (AVAudioFrameCount)(length * rate),
        (source.frameLength - startFrame));

    AVAudioPCMBuffer* segment = nil;

    if ((startFrame == 0) && (frameCount == source.frameLength)) {
        segment = source;
    } else {
        segment = [[AVAudioPCMBuffer alloc]
            initWithPCMFormat: source.format
                frameCapacity: frameCount];

        assert(segment != nil);

        for (AVAudioChannelCount channel = 0; channel < source.format.channelCount; channel++) {
            std::memcpy(
                segment.floatChannelData[channel],
                (source.floatChannelData[channel] + startFrame),
                ((std::size_t)frameCount * sizeof(float)));
        }

        segment.frameLength = frameCount;
    }

    auto node = [[AVAudioPlayerNode alloc] init];
    auto varispeed = [[AVAudioUnitVarispeed alloc] init];

    [_Engine() attachNode: node];
    [_Engine() attachNode: varispeed];
    [_Engine() connect: node to: varispeed format: source.format];
    [_Engine() connect: varispeed to: _Engine().mainMixerNode format: source.format];

    _StartEngine();

    node.volume = 1.0f;
    node.pan = 0.0f;
    varispeed.rate = 1.0f;

    _impl = new _Sound{};
    _impl->node = node;
    _impl->varispeed = varispeed;
    _impl->segment = segment;
}

void Sound::__done() {
    PROFILE

    if (_impl != nullptr) {
        [_impl->node stop];
        [_Engine() detachNode: _impl->node];
        [_Engine() detachNode: _impl->varispeed];

        delete _impl; // ARC releases the nodes and buffers
        _impl = nullptr;
    }
}

void Sound::__play() {
    PROFILE

    _impl->scheduleFrom(_loop, 0.0f);

    _paused = false;
    _impl->position = 0.0f;

    [_impl->node play];
}

void Sound::__pause() {
    PROFILE

    _impl->position = __getPosition();

    [_impl->node stop];

    _paused = true;
}

void Sound::__resume() {
    PROFILE

    _impl->scheduleFrom(_loop, _impl->position);

    _paused = false;
    _impl->position = _impl->offset;

    [_impl->node play];
}

void Sound::__stop() {
    PROFILE

    [_impl->node stop];

    _impl->releaseExtras();

    _paused = false;
    _impl->position = 0.0f;
    _impl->offset = 0.0f;
}

void Sound::__setVolume(float volume) {
    PROFILE

    _impl->node.volume = volume;
}

void Sound::__setPan(float pan) {
    PROFILE

    _impl->node.pan = pan;
}

void Sound::__setPitch(float pitch) {
    PROFILE

    _impl->varispeed.rate = std::pow(2.0f, (pitch / 12.0f));
}

void Sound::__setDecay(float decay) {
    PROFILE

    _impl->shaped = nil; /* ARC releases the previous envelope */

    if (decay <= 0.0f) {
        return;
    }

    auto segment = _impl->segment;
    auto const rate = (float)segment.format.sampleRate;
    auto const decayFrames = std::max((AVAudioFrameCount)(decay * rate), 1u);
    auto const frameCount = std::min(decayFrames, segment.frameLength);
    auto shaped = [[AVAudioPCMBuffer alloc]
        initWithPCMFormat: segment.format
            frameCapacity: frameCount];

    assert(shaped != nil);

    for (AVAudioChannelCount channel = 0; channel < segment.format.channelCount; channel++) {
        auto const input = segment.floatChannelData[channel];
        auto const output = shaped.floatChannelData[channel];

        for (AVAudioFrameCount frame = 0; frame < frameCount; frame++) {
            auto const gain = (1.0f - ((float)frame / (float)decayFrames));

            output[frame] = (input[frame] * gain * gain);
        }
    }

    shaped.frameLength = frameCount;

    _impl->shaped = shaped;
}

void Sound::__setPosition(float position) {
    PROFILE

    auto const playing = __isPlaying();

    _impl->scheduleFrom(_loop, position);

    _impl->position = _impl->offset;

    if (playing) {
        [_impl->node play];
    } else {
        _paused = true;
    }
}

float Sound::__getPosition() const {
    PROFILE

    if (_paused) {
        return _impl->position;
    }

    auto const length = _impl->segmentLength();

    if ((_impl->flag != nil) && _impl->flag->finished) {
        _impl->position = length;

        return _impl->position;
    }

    if (_impl->node.isPlaying == NO) {
        return _impl->position;
    }

    auto nodeTime = _impl->node.lastRenderTime;
    auto playerTime = ((nodeTime != nil) ? [_impl->node playerTimeForNodeTime: nodeTime] : nil);

    if (playerTime == nil) {
        return _impl->position;
    }

    auto const played = std::max((float)((double)playerTime.sampleTime / playerTime.sampleRate), 0.0f);
    auto position = (_impl->offset + played);

    if (_loop) {
        if (position > length) {
            position = std::fmod((position - length), length);
        }
    } else {
        position = std::min(position, length);
    }

    _impl->position = std::clamp(position, 0.0f, length);

    return _impl->position;
}

bool Sound::__isPlaying() const {
    PROFILE

    if (_paused) {
        return false;
    }

    if ((_impl->flag != nil) && _impl->flag->finished) {
        return false;
    }

    return (_impl->node.isPlaying == YES);
}

} /* namespace Rocket */
