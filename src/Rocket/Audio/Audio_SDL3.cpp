/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Audio/Audio.hpp>

// TODO: Implement audio on SDL3 (md/SDL3_BACKEND.md): an Audio loads nothing
// and a Sound never plays until then.

namespace Rocket {

struct Audio::_Audio {};

struct Sound::_Sound {};

void Audio::__init(std::string const& path) {
    PROFILE

    _impl = new _Audio{};
}

void Audio::__init(std::uint8_t const* data, std::size_t size) {
    PROFILE

    _impl = new _Audio{};
}

void Audio::__done() {
    PROFILE

    delete _impl;
    _impl = nullptr;
}

float Audio::__getDuration() const {
    PROFILE

    return 0.0f;
}

float Audio::__getSampleRate() const {
    PROFILE

    return 0.0f;
}

std::int32_t Audio::__getChannelCount() const {
    PROFILE

    return 0;
}

std::int64_t Audio::__getFrameCount() const {
    PROFILE

    return 0;
}

float const* Audio::__getSamples(std::int32_t channel) const {
    PROFILE

    return nullptr;
}

void Sound::__init(float start, float length) {
    PROFILE

    _impl = new _Sound{};
}

void Sound::__done() {
    PROFILE

    delete _impl;
    _impl = nullptr;
}

void Sound::__play() {
    PROFILE
}

void Sound::__pause() {
    PROFILE
}

void Sound::__resume() {
    PROFILE
}

void Sound::__stop() {
    PROFILE
}

void Sound::__setVolume(float volume) {
    PROFILE
}

void Sound::__setPan(float pan) {
    PROFILE
}

void Sound::__setPitch(float pitch) {
    PROFILE
}

void Sound::__setDecay(float decay) {
    PROFILE
}

void Sound::__setPosition(float position) {
    PROFILE
}

float Sound::__getPosition() const {
    PROFILE

    return 0.0f;
}

bool Sound::__isPlaying() const {
    PROFILE

    return false;
}

} /* namespace Rocket */
