/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <vector>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Audio/Audio.hpp>

namespace Rocket {

/** A decoded clip: its samples per channel for getSamples, and the same audio handed to the mixer. */
struct _DecodedAudio {
    SDL_AudioSpec spec = {};
    std::int64_t frameCount = 0;
    std::vector<std::vector<float>> channels;
    MIX_Audio* audio = nullptr;
};

struct Audio::_Audio : _DecodedAudio {};

/** A track playing a range of a clip, or the decay envelope made from that range. */
struct Sound::_Sound {
    MIX_Track* track = nullptr;
    MIX_Audio* shaped = nullptr;     /* the decay envelope over the range, when set */
    std::int64_t shapedFrames = 0;
    std::int64_t rangeStart = 0;     /* the range within the clip, in frames */
    std::int64_t rangeFrames = 0;
    std::int64_t activeStart = 0;    /* what the last play scheduled: the range or the envelope */
    std::int64_t activeFrames = 0;
    float rate = 0.0f;
    float position = 0.0f;           /* kept while paused, in seconds from the range start */
    bool stopped = true;

    float scheduleFrom(MIX_Audio* clip, bool loop, float from);
};

static MIX_Mixer* _mixer = nullptr;

/* One mixer on the default playback device for every sound, made on first use. */
static MIX_Mixer* _Mixer() {
    PROFILE

    if (_mixer == nullptr) {
        if (MIX_Init() == false) {
            throw std::runtime_error(SDL_GetError());
        }

        _mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

        if (_mixer == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }
    }

    return _mixer;
}

/* Decodes a whole clip to float32 with SDL_mixer's decoders, then hands it to the mixer. */
static void _Decode(_DecodedAudio& audio, MIX_AudioDecoder* decoder, std::string const& name) {
    PROFILE

    if (decoder == nullptr) {
        throw std::runtime_error("Failed to load audio: " + name + ": " + SDL_GetError());
    }

    auto spec = SDL_AudioSpec{};

    if (MIX_GetAudioDecoderFormat(decoder, &spec) == false) {
        MIX_DestroyAudioDecoder(decoder);
        throw std::runtime_error("Failed to decode audio: " + name);
    }

    spec.format = SDL_AUDIO_F32;

    auto interleaved = std::vector<float>();
    auto chunk = std::vector<float>(static_cast<std::size_t>(spec.channels) * 4096);

    for (;;) {
        auto const bytes = MIX_DecodeAudio(decoder, chunk.data(), static_cast<int>(chunk.size() * sizeof(float)), &spec);

        if (bytes < 0) {
            MIX_DestroyAudioDecoder(decoder);
            throw std::runtime_error("Failed to decode audio: " + name);
        }

        if (bytes == 0) {
            break;
        }

        interleaved.insert(interleaved.end(), chunk.begin(), (chunk.begin() + (bytes / static_cast<int>(sizeof(float)))));
    }

    MIX_DestroyAudioDecoder(decoder);

    audio.spec = spec;
    audio.frameCount = static_cast<std::int64_t>(interleaved.size() / static_cast<std::size_t>(spec.channels));
    audio.channels.assign(static_cast<std::size_t>(spec.channels), std::vector<float>(static_cast<std::size_t>(audio.frameCount)));

    for (auto frame = std::int64_t(0); frame < audio.frameCount; frame++) {
        for (auto channel = 0; channel < spec.channels; channel++) {
            audio.channels[static_cast<std::size_t>(channel)][static_cast<std::size_t>(frame)] = interleaved[static_cast<std::size_t>((frame * spec.channels) + channel)];
        }
    }

    audio.audio = MIX_LoadRawAudio(_Mixer(), interleaved.data(), (interleaved.size() * sizeof(float)), &spec);

    if (audio.audio == nullptr) {
        throw std::runtime_error("Failed to load audio: " + name);
    }
}

/* Plays the range, or the decay envelope when one is set, from a position in
   seconds; one past the end starts over. Returns where it started. */
float Sound::_Sound::scheduleFrom(MIX_Audio* clip, bool loop, float from) {
    PROFILE

    auto const audio = (shaped != nullptr) ? shaped : clip;

    activeStart = (shaped != nullptr) ? 0 : rangeStart;
    activeFrames = (shaped != nullptr) ? shapedFrames : rangeFrames;

    auto startFrame = static_cast<std::int64_t>(from * rate);

    if (startFrame >= activeFrames) {
        startFrame = 0;
    }

    MIX_SetTrackAudio(track, audio);

    auto const options = SDL_CreateProperties();

    SDL_SetNumberProperty(options, MIX_PROP_PLAY_START_FRAME_NUMBER, (activeStart + startFrame));
    SDL_SetNumberProperty(options, MIX_PROP_PLAY_MAX_FRAME_NUMBER, (activeStart + activeFrames));
    SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOP_START_FRAME_NUMBER, activeStart);
    SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, (loop ? -1 : 0));

    MIX_PlayTrack(track, options);
    SDL_DestroyProperties(options);

    stopped = false;

    return (static_cast<float>(startFrame) / rate);
}

void Audio::__init(std::string const& path) {
    PROFILE

    _Mixer(); /* the decoders need SDL_mixer initialized */

    _impl = new _Audio{};

    try {
        _Decode(*_impl, MIX_CreateAudioDecoder(path.c_str(), 0), path);
    } catch (...) {
        delete _impl;
        _impl = nullptr;
        throw;
    }
}

void Audio::__init(std::uint8_t const* data, std::size_t size) {
    PROFILE

    _Mixer(); /* the decoders need SDL_mixer initialized */

    _impl = new _Audio{};

    try {
        _Decode(*_impl, MIX_CreateAudioDecoder_IO(SDL_IOFromConstMem(data, size), true, 0), "audio data");
    } catch (...) {
        delete _impl;
        _impl = nullptr;
        throw;
    }
}

void Audio::__done() {
    PROFILE

    if (_impl != nullptr) {
        MIX_DestroyAudio(_impl->audio);
        delete _impl;
        _impl = nullptr;
    }
}

float Audio::__getDuration() const {
    PROFILE

    return (static_cast<float>(_impl->frameCount) / static_cast<float>(_impl->spec.freq));
}

float Audio::__getSampleRate() const {
    PROFILE

    return static_cast<float>(_impl->spec.freq);
}

std::int32_t Audio::__getChannelCount() const {
    PROFILE

    return static_cast<std::int32_t>(_impl->spec.channels);
}

std::int64_t Audio::__getFrameCount() const {
    PROFILE

    return _impl->frameCount;
}

float const* Audio::__getSamples(std::int32_t channel) const {
    PROFILE

    return _impl->channels[static_cast<std::size_t>(channel)].data();
}

void Sound::__init(float start, float length) {
    PROFILE

    auto const& clip = *_audio->_impl;
    auto const rate = static_cast<float>(clip.spec.freq);
    auto const rangeStart = std::min(static_cast<std::int64_t>(start * rate), clip.frameCount);
    auto const rangeFrames = std::min(static_cast<std::int64_t>(length * rate), (clip.frameCount - rangeStart));
    auto const track = MIX_CreateTrack(_Mixer());

    if (track == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    _impl = new _Sound{};
    _impl->track = track;
    _impl->rangeStart = rangeStart;
    _impl->rangeFrames = rangeFrames;
    _impl->activeStart = rangeStart;
    _impl->activeFrames = rangeFrames;
    _impl->rate = rate;
}

void Sound::__done() {
    PROFILE

    if (_impl != nullptr) {
        MIX_DestroyTrack(_impl->track);
        MIX_DestroyAudio(_impl->shaped);
        delete _impl;
        _impl = nullptr;
    }
}

void Sound::__play() {
    PROFILE

    _impl->scheduleFrom(_audio->_impl->audio, _loop, 0.0f);

    _paused = false;
    _impl->position = 0.0f;
}

void Sound::__pause() {
    PROFILE

    _impl->position = __getPosition();

    MIX_PauseTrack(_impl->track);

    _paused = true;
}

void Sound::__resume() {
    PROFILE

    /* scheduled again, as on macOS: the loop and decay set since play() apply */
    _impl->scheduleFrom(_audio->_impl->audio, _loop, _impl->position);

    _paused = false;
}

void Sound::__stop() {
    PROFILE

    MIX_StopTrack(_impl->track, 0);

    _paused = false;
    _impl->stopped = true;
    _impl->position = 0.0f;
}

void Sound::__setVolume(float volume) {
    PROFILE

    MIX_SetTrackGain(_impl->track, volume);
}

void Sound::__setPan(float pan) {
    PROFILE

    /* A balance: the side panned away from fades out, the other stays at
       full gain, so the center leaves the sound as it is. */
    if (pan == 0.0f) {
        MIX_SetTrackStereo(_impl->track, nullptr);
    } else {
        auto const gains = MIX_StereoGains{
            .left = std::min(1.0f, (1.0f - pan)),
            .right = std::min(1.0f, (1.0f + pan))
        };

        MIX_SetTrackStereo(_impl->track, &gains);
    }
}

void Sound::__setPitch(float pitch) {
    PROFILE

    /* the frequency ratio shifts speed and pitch together, as macOS's varispeed does */
    MIX_SetTrackFrequencyRatio(_impl->track, std::clamp(std::pow(2.0f, (pitch / 12.0f)), 0.01f, 100.0f));
}

void Sound::__setDecay(float decay) {
    PROFILE

    MIX_DestroyAudio(_impl->shaped); /* a track still playing it keeps it until it stops */

    _impl->shaped = nullptr;
    _impl->shapedFrames = 0;

    if (decay <= 0.0f) {
        return;
    }

    auto const& clip = *_audio->_impl;
    auto const channels = static_cast<std::int64_t>(clip.spec.channels);
    auto const decayFrames = std::max(static_cast<std::int64_t>(decay * _impl->rate), std::int64_t(1));
    auto const frameCount = std::min(decayFrames, _impl->rangeFrames);
    auto shaped = std::vector<float>(static_cast<std::size_t>(frameCount * channels));

    for (auto frame = std::int64_t(0); frame < frameCount; frame++) {
        auto const gain = (1.0f - (static_cast<float>(frame) / static_cast<float>(decayFrames)));

        for (auto channel = std::int64_t(0); channel < channels; channel++) {
            auto const input = clip.channels[static_cast<std::size_t>(channel)][static_cast<std::size_t>(_impl->rangeStart + frame)];

            shaped[static_cast<std::size_t>((frame * channels) + channel)] = (input * gain * gain);
        }
    }

    _impl->shaped = MIX_LoadRawAudio(_Mixer(), shaped.data(), (shaped.size() * sizeof(float)), &clip.spec);
    _impl->shapedFrames = ((_impl->shaped != nullptr) ? frameCount : 0);
}

void Sound::__setPosition(float position) {
    PROFILE

    auto const playing = __isPlaying();

    /* a sound that is not playing starts paused at the position: the mixer is
       locked so that none of it is mixed in between */
    MIX_LockMixer(_Mixer());

    _impl->position = _impl->scheduleFrom(_audio->_impl->audio, _loop, position);

    if (playing == false) {
        MIX_PauseTrack(_impl->track);
    }

    MIX_UnlockMixer(_Mixer());

    if (playing == false) {
        _paused = true;
    }
}

float Sound::__getPosition() const {
    PROFILE

    if (_paused) {
        return _impl->position;
    }

    if (_impl->stopped) {
        return 0.0f;
    }

    /* the track's position counts frames of the clip itself, before the frequency ratio */
    auto const frames = (MIX_GetTrackPlaybackPosition(_impl->track) - _impl->activeStart);

    return (static_cast<float>(std::clamp(frames, std::int64_t(0), _impl->activeFrames)) / _impl->rate);
}

bool Sound::__isPlaying() const {
    PROFILE

    if (_paused) {
        return false;
    }

    return MIX_TrackPlaying(_impl->track);
}

} /* namespace Rocket */
