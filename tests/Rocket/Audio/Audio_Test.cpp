/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <Rocket/Audio/Audio.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static float const _ToneMs = 500.0f;
static int const _ToneRate = 44100;

/* Effects tests use a barely audible volume so running the suite is not annoying. */
static float const _QuietVolume = 0.05f;

static std::string _TonePath() {
    return std::string(ROCKET_BINARY_DIR) + "/audio-test.wav";
}

/* Writes a 500ms 440Hz mono 16-bit WAV to `path` and returns its bytes. */
static std::vector<std::uint8_t> _WriteToneWav(std::string const& path) {
    auto const count = (int)((_ToneMs / 1000.0f) * (float)_ToneRate);
    auto const dataSize = ((std::size_t)count * 2);
    auto const fileSize = (44 + dataSize);
    auto buffer = std::vector<std::uint8_t>(fileSize);

    auto const chunkSize = (std::uint32_t)(fileSize - 8);
    auto const formatSize = (std::uint32_t)16;
    auto const format = (std::uint16_t)1;
    auto const channels = (std::uint16_t)1;
    auto const rate = (std::uint32_t)_ToneRate;
    auto const byteRate = (std::uint32_t)(rate * 2);
    auto const blockAlign = (std::uint16_t)2;
    auto const bits = (std::uint16_t)16;
    auto const dataSize32 = (std::uint32_t)dataSize;

    std::memcpy(buffer.data(), "RIFF", 4);
    std::memcpy((buffer.data() + 4), &chunkSize, 4);
    std::memcpy((buffer.data() + 8), "WAVE", 4);
    std::memcpy((buffer.data() + 12), "fmt ", 4);
    std::memcpy((buffer.data() + 16), &formatSize, 4);
    std::memcpy((buffer.data() + 20), &format, 2);
    std::memcpy((buffer.data() + 22), &channels, 2);
    std::memcpy((buffer.data() + 24), &rate, 4);
    std::memcpy((buffer.data() + 28), &byteRate, 4);
    std::memcpy((buffer.data() + 32), &blockAlign, 2);
    std::memcpy((buffer.data() + 34), &bits, 2);
    std::memcpy((buffer.data() + 36), "data", 4);
    std::memcpy((buffer.data() + 40), &dataSize32, 4);

    for (int i = 0; i < count; i++) {
        auto const sample = (std::int16_t)(std::sin(((float)i / (float)rate) * 440.0f * 2.0f * (float)M_PI) * 12000.0f);

        std::memcpy((buffer.data() + 44 + ((std::size_t)i * 2)), &sample, 2);
    }

    auto file = std::fopen(path.c_str(), "wb");
    EXPECT_TRUE(file != nullptr) << "cannot open " << path;

    if (file == nullptr) {
        return buffer;
    }

    auto const written = std::fwrite(buffer.data(), 1, fileSize, file);
    std::fclose(file);
    EXPECT_EQ(written, fileSize);

    return buffer;
}

static void _Sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

/* Both constructors decode the same clip to the same duration. */
TEST(Audio, DecodesFileAndData) {
    auto const bytes = _WriteToneWav(_TonePath());

    auto fromFile = Audio(_TonePath());
    EXPECT_NEAR(fromFile.getDuration(), _ToneMs, 11.0f);

    auto fromData = Audio(bytes.data(), bytes.size());
    EXPECT_NEAR(fromData.getDuration(), _ToneMs, 11.0f);
}

/* The decoded samples are exposed per channel, in [-1, 1], at the file's rate. */
TEST(Audio, ExposesSamples) {
    _WriteToneWav(_TonePath());

    auto audio = Audio(_TonePath());
    EXPECT_FLOAT_EQ(audio.getSampleRate(), (float)_ToneRate);
    EXPECT_EQ(audio.getChannelCount(), 1);
    EXPECT_EQ(audio.getFrameCount(), (std::int64_t)((_ToneMs / 1000.0f) * (float)_ToneRate));

    auto const samples = audio.getSamples(0);
    ASSERT_NE(samples, nullptr);

    auto peak = 0.0f;
    for (std::int64_t i = 0; i < audio.getFrameCount(); i++) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    /* The tone was written at 12000/32768 of full scale. */
    EXPECT_NEAR(peak, (12000.0f / 32768.0f), 0.01f);

    EXPECT_THROW(audio.getSamples(1), std::out_of_range);
    EXPECT_THROW(audio.getSamples(-1), std::out_of_range);
}

/* Ranges, volume, looping and the play/pause/resume/stop lifecycle. */
TEST(Sound, RangesAndControls) {
    _WriteToneWav(_TonePath());

    auto audio = Audio(_TonePath());
    auto whole = Sound(audio);
    auto part = Sound(audio, 100.0f, 200.0f);

    EXPECT_EQ(&whole.getAudio(), &audio);
    EXPECT_EQ(whole.getStart(), 0.0f);
    EXPECT_NEAR(whole.getDuration(), _ToneMs, 11.0f);
    EXPECT_NEAR(part.getStart(), 100.0f, 11.0f);
    EXPECT_NEAR(part.getDuration(), 200.0f, 11.0f);

    EXPECT_EQ(whole.getVolume(), 1.0f);
    whole.setVolume(_QuietVolume);
    EXPECT_NEAR(whole.getVolume(), _QuietVolume, 0.011f);
    part.setVolume(_QuietVolume);

    EXPECT_FALSE(whole.isLooping());
    whole.setLoop(true);
    EXPECT_TRUE(whole.isLooping());
    whole.setLoop(false);

    EXPECT_FALSE(whole.isPlaying());
    EXPECT_FALSE(whole.isPaused());

    whole.play();
    part.play();
    EXPECT_TRUE(whole.isPlaying());
    EXPECT_TRUE(part.isPlaying());

    whole.pause();
    EXPECT_TRUE(whole.isPaused());
    EXPECT_FALSE(whole.isPlaying());
    EXPECT_TRUE(part.isPlaying());

    whole.resume();
    EXPECT_FALSE(whole.isPaused());
    EXPECT_TRUE(whole.isPlaying());

    whole.stop();
    part.stop();
    EXPECT_FALSE(whole.isPlaying());
    EXPECT_EQ(whole.getPosition(), 0.0f);
}

/* A finished one-shot stops at its range end; a looping sound keeps going. */
TEST(Sound, FinishesAndLoops) {
    _WriteToneWav(_TonePath());

    auto audio = Audio(_TonePath());
    auto blip = Sound(audio, 0.0f, 100.0f);

    blip.setVolume(_QuietVolume);
    blip.play();
    EXPECT_TRUE(blip.isPlaying());

    _Sleep(300);
    EXPECT_FALSE(blip.isPlaying());
    EXPECT_NEAR(blip.getPosition(), 100.0f, 11.0f);

    blip.setLoop(true);
    blip.play();
    _Sleep(300);
    EXPECT_TRUE(blip.isPlaying());
    EXPECT_LE(blip.getPosition(), 100.0f);

    blip.stop();
    EXPECT_FALSE(blip.isPlaying());
}

/* Seeking pauses a stopped sound at the position and jumps a playing one. */
TEST(Sound, SeeksWithinRange) {
    _WriteToneWav(_TonePath());

    auto audio = Audio(_TonePath());
    auto sound = Sound(audio);

    sound.setVolume(_QuietVolume);

    sound.setPosition(300.0f);
    EXPECT_TRUE(sound.isPaused());
    EXPECT_FALSE(sound.isPlaying());
    EXPECT_NEAR(sound.getPosition(), 300.0f, 11.0f);

    sound.resume();
    EXPECT_TRUE(sound.isPlaying());
    EXPECT_GE(sound.getPosition(), 290.0f);

    sound.setPosition(100.0f);
    EXPECT_TRUE(sound.isPlaying());

    _Sleep(50);

    auto const position = sound.getPosition();
    EXPECT_GE(position, 90.0f);
    EXPECT_LE(position, 300.0f);

    sound.stop();
    EXPECT_EQ(sound.getPosition(), 0.0f);
}

/* Pan clamps, pitch doubles the playback speed at +12, decay shortens the range. */
TEST(Sound, AppliesEffects) {
    _WriteToneWav(_TonePath());

    auto audio = Audio(_TonePath());
    auto sound = Sound(audio);

    sound.setVolume(_QuietVolume);

    EXPECT_EQ(sound.getPan(), 0.0f);
    sound.setPan(0.5f);
    EXPECT_NEAR(sound.getPan(), 0.5f, 0.011f);
    sound.setPan(-2.0f);
    EXPECT_NEAR(sound.getPan(), -1.0f, 0.011f);
    sound.setPan(0.0f);

    EXPECT_EQ(sound.getPitch(), 0.0f);
    sound.setPitch(12.0f);
    EXPECT_NEAR(sound.getPitch(), 12.0f, 0.011f);

    /* At +12 semitones the position advances at double speed, so after 250ms
       of wall clock it must be well past the 250ms a normal-rate sound would
       reach. */
    sound.play();
    _Sleep(250);
    EXPECT_GT(sound.getPosition(), 350.0f);
    sound.stop();

    sound.setPitch(0.0f);
    EXPECT_EQ(sound.getPitch(), 0.0f);

    EXPECT_EQ(sound.getDecay(), 0.0f);
    sound.setDecay(100.0f);
    EXPECT_NEAR(sound.getDecay(), 100.0f, 11.0f);

    sound.play();
    EXPECT_TRUE(sound.isPlaying());
    _Sleep(300);
    EXPECT_FALSE(sound.isPlaying());
    EXPECT_NEAR(sound.getPosition(), 100.0f, 11.0f);

    sound.setDecay(0.0f);
    EXPECT_EQ(sound.getDecay(), 0.0f);

    sound.play();
    _Sleep(300);
    EXPECT_TRUE(sound.isPlaying());

    sound.stop();
}
