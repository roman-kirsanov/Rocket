/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Audio/Audio.hpp>

namespace Rocket {

Audio::~Audio() {
    PROFILE

    __done();
}

Audio::Audio(std::string const& path)
    : _impl(nullptr)
    , _duration(0.0f)
{
    PROFILE

    __init(path);

    _duration = (__getDuration() * 1000.0f);
}

Audio::Audio(std::uint8_t const* data, std::size_t size)
    : _impl(nullptr)
    , _duration(0.0f)
{
    PROFILE

    if ((data == nullptr) || (size == 0)) {
        throw std::runtime_error("`data` is required");
    }

    __init(data, size);

    _duration = (__getDuration() * 1000.0f);
}

float Audio::getDuration() const {
    PROFILE

    return _duration;
}

float Audio::getSampleRate() const {
    PROFILE

    return __getSampleRate();
}

std::int32_t Audio::getChannelCount() const {
    PROFILE

    return __getChannelCount();
}

std::int64_t Audio::getFrameCount() const {
    PROFILE

    return __getFrameCount();
}

float const* Audio::getSamples(std::int32_t channel) const {
    PROFILE

    if ((channel < 0) || (channel >= __getChannelCount())) {
        throw std::out_of_range("Audio channel index out of range");
    }

    return __getSamples(channel);
}

Sound::~Sound() {
    PROFILE

    __done();
}

Sound::Sound(Audio& audio, float start, float length)
    : _impl(nullptr)
    , _audio(&audio)
    , _start(start)
    , _length(0.0f)
    , _volume(1.0f)
    , _pan(0.0f)
    , _pitch(0.0f)
    , _decay(0.0f)
    , _loop(false)
    , _paused(false)
{
    PROFILE

    assert(audio._impl != nullptr);
    assert(start >= 0.0f);
    assert(start < audio.getDuration());
    assert(length >= 0.0f);

    auto const remaining = (audio.getDuration() - start);

    _length = ((length > 0.0f) ? std::min(length, remaining) : remaining);

    __init((_start / 1000.0f), (_length / 1000.0f));
}

void Sound::play() {
    PROFILE

    assert(_impl != nullptr);

    __play();
}

void Sound::pause() {
    PROFILE

    assert(_impl != nullptr);

    if (isPlaying()) {
        __pause();
    }
}

void Sound::resume() {
    PROFILE

    assert(_impl != nullptr);

    if (_paused) {
        __resume();
    }
}

void Sound::stop() {
    PROFILE

    assert(_impl != nullptr);

    __stop();
}

void Sound::setLoop(bool loop) {
    PROFILE

    _loop = loop;
}

bool Sound::isLooping() const {
    PROFILE

    return _loop;
}

void Sound::setVolume(float volume) {
    PROFILE

    assert(_impl != nullptr);

    _volume = std::clamp(volume, 0.0f, 1.0f);

    __setVolume(_volume);
}

float Sound::getVolume() const {
    PROFILE

    return _volume;
}

void Sound::setPan(float pan) {
    PROFILE

    assert(_impl != nullptr);

    _pan = std::clamp(pan, -1.0f, 1.0f);

    __setPan(_pan);
}

float Sound::getPan() const {
    PROFILE

    return _pan;
}

void Sound::setPitch(float pitch) {
    PROFILE

    assert(_impl != nullptr);

    _pitch = pitch;

    __setPitch(_pitch);
}

float Sound::getPitch() const {
    PROFILE

    return _pitch;
}

void Sound::setDecay(float decay) {
    PROFILE

    assert(_impl != nullptr);
    assert(decay >= 0.0f);

    _decay = decay;

    __setDecay(_decay / 1000.0f);
}

float Sound::getDecay() const {
    PROFILE

    return _decay;
}

void Sound::setPosition(float position) {
    PROFILE

    assert(_impl != nullptr);

    __setPosition(std::clamp(position, 0.0f, _length) / 1000.0f);
}

float Sound::getPosition() const {
    PROFILE

    assert(_impl != nullptr);

    return (__getPosition() * 1000.0f);
}

float Sound::getDuration() const {
    PROFILE

    return _length;
}

float Sound::getStart() const {
    PROFILE

    return _start;
}

Audio& Sound::getAudio() const {
    PROFILE

    return *_audio;
}

bool Sound::isPlaying() const {
    PROFILE

    assert(_impl != nullptr);

    return __isPlaying();
}

bool Sound::isPaused() const {
    PROFILE

    return _paused;
}

} /* namespace Rocket */
