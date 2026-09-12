/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace Rocket {

class Sound;

/**
 * A decoded audio clip.
 *
 * Non-copyable and non-movable. The whole file is decoded into memory at
 * construction time. Played through Sound instances that reference the clip;
 * sounds created over this audio must be destroyed first.
 */
class Audio {
public:
    ~Audio();

    /**
     * Constructs an audio clip by loading and decoding an audio file.
     * Throws std::runtime_error if the file cannot be loaded or decoded.
     *
     * @param path Path to the audio file (WAV, MP3, M4A, AIFF, ...).
     */
    Audio(std::string const& path);

    /**
     * Constructs an audio clip by decoding raw audio file bytes.
     * Throws std::runtime_error if the bytes cannot be decoded.
     *
     * @param data Raw audio file bytes (WAV, MP3, M4A, AIFF, ...).
     * @param size Data size in bytes.
     */
    Audio(std::uint8_t const* data, std::size_t size);

    Audio(Audio &&) = delete;
    Audio(Audio const&) = delete;
    Audio& operator=(Audio &&) = delete;
    Audio& operator=(Audio const&) = delete;

    /** Returns the decoded clip duration in milliseconds. */
    float getDuration() const;

    /** Returns the decoded sample rate in frames per second. */
    float getSampleRate() const;

    /** Returns the number of decoded channels (1 for mono, 2 for stereo, ...). */
    std::int32_t getChannelCount() const;

    /** Returns the number of decoded frames (samples per channel). */
    std::int64_t getFrameCount() const;

    /**
     * Returns the decoded samples of one channel as getFrameCount() floats
     * in [-1, 1], valid for the lifetime of the audio. Channels are stored
     * non-interleaved, so each channel is a separate contiguous array.
     * Throws std::out_of_range if the channel does not exist.
     *
     * @param channel Zero-based channel index.
     */
    float const* getSamples(std::int32_t channel) const;
private:
    struct _Audio;
    _Audio* _impl;
    float _duration;

    void __init(std::string const&);
    void __init(std::uint8_t const*, std::size_t);
    void __done();
    float __getDuration() const;
    float __getSampleRate() const;
    std::int32_t __getChannelCount() const;
    std::int64_t __getFrameCount() const;
    float const* __getSamples(std::int32_t) const;

    friend class Sound;
};

/**
 * A playback channel over a clip range of an Audio, fixed at construction.
 *
 * Non-copyable and non-movable. The referenced Audio is not owned and must
 * outlive the sound. All times are in milliseconds.
 */
class Sound {
public:
    ~Sound();

    /**
     * Constructs a playback channel over a clip range of the audio. The
     * range is fixed for the sound's lifetime.
     *
     * @param audio  The audio to play from; must outlive the sound.
     * @param start  Range start in milliseconds.
     * @param length Range length in milliseconds; 0 plays to the end of
     *               the clip.
     */
    Sound(Audio& audio, float start = 0.0f, float length = 0.0f);

    Sound(Sound &&) = delete;
    Sound(Sound const&) = delete;
    Sound& operator=(Sound &&) = delete;
    Sound& operator=(Sound const&) = delete;

    /**
     * Starts the sound from the beginning of its range, restarting it when
     * it is already playing.
     */
    void play();

    /**
     * Pauses the sound, keeping the playback position; does nothing when
     * not playing.
     */
    void pause();

    /**
     * Resumes a paused sound from its kept position; does nothing when not
     * paused.
     */
    void resume();

    /** Stops the sound and rewinds it to the beginning of its range. */
    void stop();

    /**
     * Sets whether the sound repeats its range; takes effect on the next
     * play().
     *
     * @param loop True to repeat until stop().
     */
    void setLoop(bool loop);

    /** Returns whether the sound is set to repeat its range. */
    bool isLooping() const;

    /**
     * Sets the sound volume, applied immediately.
     *
     * @param volume Volume from 0 (silent) to 1 (full).
     */
    void setVolume(float volume);

    /** Returns the volume from 0 (silent) to 1 (full). */
    float getVolume() const;

    /**
     * Sets the stereo pan of the sound, applied immediately.
     *
     * @param pan Pan from -1 (left) through 0 (center) to 1 (right).
     */
    void setPan(float pan);

    /** Returns the pan from -1 (left) through 0 (center) to 1 (right). */
    float getPan() const;

    /**
     * Sets the pitch of the sound in semitones, resampling like a classic
     * sampler — the playback speed shifts together with the pitch; applied
     * immediately.
     *
     * @param pitch Pitch offset in semitones (positive is higher and faster).
     */
    void setPitch(float pitch);

    /** Returns the pitch offset in semitones. */
    float getPitch() const;

    /**
     * Sets a fade-out envelope over the start of the sound range: the gain
     * falls from full to silent across the decay time and playback stops
     * there; 0 disables the envelope and plays the natural range; takes
     * effect on the next play().
     *
     * @param decay Fade-out time in milliseconds, 0 for the natural range.
     */
    void setDecay(float decay);

    /** Returns the fade-out envelope time in milliseconds, 0 when disabled. */
    float getDecay() const;

    /**
     * Sets the playback position within the sound range; a playing sound
     * jumps there immediately, any other sound becomes paused at that
     * position so resume() continues from it.
     *
     * @param position Position in milliseconds from the range start.
     */
    void setPosition(float position);

    /**
     * Returns the playback position in milliseconds from the range start.
     * The position is kept while paused, is 0 while stopped and equals the
     * range length after a finished play.
     */
    float getPosition() const;

    /** Returns the length of the sound range in milliseconds. */
    float getDuration() const;

    /** Returns the start of the sound range within the audio clip in milliseconds. */
    float getStart() const;

    /** Returns the audio the sound plays from. */
    Audio& getAudio() const;

    /**
     * Returns true while the sound is audibly playing (not paused, not
     * finished, not stopped).
     */
    bool isPlaying() const;

    /** Returns true while the sound is paused with a kept position. */
    bool isPaused() const;
private:
    struct _Sound;
    _Sound* _impl;
    Audio* _audio;
    float _start;
    float _length;
    float _volume;
    float _pan;
    float _pitch;
    float _decay;
    bool _loop;
    bool _paused;

    void __init(float, float);
    void __done();
    void __play();
    void __pause();
    void __resume();
    void __stop();
    void __setVolume(float);
    void __setPan(float);
    void __setPitch(float);
    void __setDecay(float);
    void __setPosition(float);
    float __getPosition() const;
    bool __isPlaying() const;
};

} /* namespace Rocket */
