/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <cmath>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <windows.h>
#include <objbase.h>
#include <wrl/client.h>
#include <shlwapi.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Audio/Audio.hpp>

using Microsoft::WRL::ComPtr;

namespace Rocket {

static auto constexpr _BUFFER_DURATION = REFERENCE_TIME(200000); // 20 ms, in 100 ns units

/* Without a device the mixer still runs, on a clock at this rate. */
static auto constexpr _SILENT_RATE = 48000;
static auto constexpr _SILENT_PERIOD_MS = 10;

struct _DecodedAudio {
    std::int32_t rate = 0;
    std::int32_t channelCount = 0;
    std::int64_t frameCount = 0;
    std::vector<std::vector<float>> channels;
};

struct Audio::_Audio : _DecodedAudio {};

/** Guarded by the mixer's lock. */
struct _Voice {
    _DecodedAudio const* clip = nullptr;
    std::int64_t rangeStart = 0;   /* the range within the clip, in frames */
    std::int64_t rangeFrames = 0;
    std::int64_t decayFrames = 0;  /* the decay set, applied from the next play or resume */
    std::int64_t activeFrames = 0; /* what the last play scheduled: the range, or the decay envelope over it */
    std::int64_t activeDecay = 0;
    bool loop = false;
    double cursor = 0.0;           /* in clip frames, from the range start */
    float volume = 1.0f;
    float left = 1.0f;
    float right = 1.0f;
    float ratio = 1.0f;
    bool playing = false;
    bool stopped = true;
    float position = 0.0f;         /* kept while paused, in seconds from the range start */
};

struct Sound::_Sound : _Voice {};

/** Its calls come on a thread of their own and must not block. */
class _DeviceNotifications final : public IMMNotificationClient {
public:
    explicit _DeviceNotifications(HANDLE event) : _event(event) {}

    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** object) override {
        if ((id == __uuidof(IUnknown)) || (id == __uuidof(IMMNotificationClient))) {
            *object = static_cast<IMMNotificationClient*>(this);
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override {
        if ((flow == eRender) && (role == eConsole)) {
            ::SetEvent(_event);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, PROPERTYKEY const) override { return S_OK; }

private:
    HANDLE _event;
};

struct _Mixer {
    std::mutex mutex;
    std::vector<_Voice*> voices;
    std::thread thread;
    HANDLE quit = nullptr;
    HANDLE deviceChanged = nullptr;
    std::atomic<std::int32_t> rate = _SILENT_RATE;
    std::vector<float> scratch;

    void mix(float* output, std::uint32_t frames, std::int32_t outputRate);
    void run();
    bool render(IMMDeviceEnumerator* enumerator);
    void renderSilently();
};

static _Mixer* _mixer = nullptr;

/* The gain of the decay envelope at a frame of it: a quadratic fade to silence. */
static float _DecayGain(double frame, std::int64_t decayFrames) {
    PROFILE

    auto const linear = 1.0f - static_cast<float>(frame / static_cast<double>(decayFrames));

    return linear * linear;
}

static float _SampleAt(std::vector<float> const& samples, std::int64_t first, std::int64_t last, double frame) {
    PROFILE

    auto const index = std::clamp(static_cast<std::int64_t>(frame), first, last);
    auto const next = std::min(index + 1, last);
    auto const fraction = static_cast<float>(frame - std::floor(frame));

    return samples[static_cast<std::size_t>(index)] + ((samples[static_cast<std::size_t>(next)] - samples[static_cast<std::size_t>(index)]) * fraction);
}

/* Interleaved stereo float32, as the device stream is opened with. */
void _Mixer::mix(float* output, std::uint32_t frames, std::int32_t outputRate) {
    PROFILE

    std::fill(output, (output + (static_cast<std::size_t>(frames) * 2)), 0.0f);

    auto lock = std::scoped_lock(mutex);

    for (auto const voice : voices) {
        if (voice->playing == false) {
            continue;
        }

        auto const& clip = *voice->clip;
        auto const step = (static_cast<double>(clip.rate) / static_cast<double>(outputRate)) * static_cast<double>(voice->ratio);
        auto const first = voice->rangeStart;
        auto const last = std::max(first, (voice->rangeStart + voice->activeFrames - 1));
        auto const stereo = (clip.channelCount >= 2);

        for (auto frame = std::uint32_t(0); frame < frames; frame++) {
            if (voice->cursor >= static_cast<double>(voice->activeFrames)) {
                if (voice->loop && (voice->activeFrames > 0)) {
                    voice->cursor = std::fmod(voice->cursor, static_cast<double>(voice->activeFrames));
                } else {
                    /* a one-shot ends on its last frame, and keeps that position */
                    voice->cursor = static_cast<double>(voice->activeFrames);
                    voice->playing = false;
                    break;
                }
            }

            auto const at = static_cast<double>(first) + voice->cursor;
            auto gain = voice->volume;

            if (voice->activeDecay > 0) {
                gain *= _DecayGain(voice->cursor, voice->activeDecay);
            }

            auto const left = _SampleAt(clip.channels[0], first, last, at);
            auto const right = stereo ? _SampleAt(clip.channels[1], first, last, at) : left;

            output[(frame * 2) + 0] += (left * gain * voice->left);
            output[(frame * 2) + 1] += (right * gain * voice->right);

            voice->cursor += step;
        }
    }

    for (auto i = std::size_t(0); i < (static_cast<std::size_t>(frames) * 2); i++) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

/* Plays through the default device until it goes away or changes; false
   when there is none to play through. */
bool _Mixer::render(IMMDeviceEnumerator* enumerator) {
    PROFILE

    auto device = ComPtr<IMMDevice>();
    auto client = ComPtr<IAudioClient>();
    auto renderClient = ComPtr<IAudioRenderClient>();
    auto mixFormat = static_cast<WAVEFORMATEX*>(nullptr);

    if (
        FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device)) ||
        FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &client)) ||
        FAILED(client->GetMixFormat(&mixFormat))
    ) {
        return false;
    }

    /* stereo float32 at the rate the device mixes at; Windows converts it to
       the device's own channel layout */
    auto const deviceRate = static_cast<std::int32_t>(mixFormat->nSamplesPerSec);
    ::CoTaskMemFree(mixFormat);

    auto format = WAVEFORMATEX{};
    format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels = 2;
    format.nSamplesPerSec = static_cast<DWORD>(deviceRate);
    format.wBitsPerSample = 32;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = (format.nSamplesPerSec * format.nBlockAlign);
    format.cbSize = 0;

    auto const flags = (AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY);
    auto const ready = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    auto bufferFrames = UINT32(0);

    if (
        FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, _BUFFER_DURATION, 0, &format, nullptr)) ||
        FAILED(client->SetEventHandle(ready)) ||
        FAILED(client->GetBufferSize(&bufferFrames)) ||
        FAILED(client->GetService(IID_PPV_ARGS(&renderClient))) ||
        FAILED(client->Start())
    ) {
        ::CloseHandle(ready);
        return false;
    }

    rate = deviceRate;

    HANDLE const handles[] = { ready, quit, deviceChanged };

    for (;;) {
        auto const signaled = ::WaitForMultipleObjects(3, handles, FALSE, 1000);

        if (signaled == (WAIT_OBJECT_0 + 1)) {
            break;
        }

        if (signaled == (WAIT_OBJECT_0 + 2)) {
            break; /* opened again on the new default device */
        }

        auto padding = UINT32(0);

        if (FAILED(client->GetCurrentPadding(&padding))) {
            break; /* the device went away: opened again */
        }

        auto const frames = (bufferFrames - padding);
        auto data = static_cast<BYTE*>(nullptr);

        if ((frames == 0) || FAILED(renderClient->GetBuffer(frames, &data))) {
            continue;
        }

        mix(reinterpret_cast<float*>(data), frames, deviceRate);
        renderClient->ReleaseBuffer(frames, 0);
    }

    client->Stop();
    ::CloseHandle(ready);

    return true;
}

/* No device to play through (a machine without sound, a remote session):
   the voices play on a clock all the same, so positions and endings keep
   real time, until a device appears. */
void _Mixer::renderSilently() {
    PROFILE

    auto const timer = ::CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    auto frequency = LARGE_INTEGER{};
    auto last = LARGE_INTEGER{};

    ::QueryPerformanceFrequency(&frequency);
    ::QueryPerformanceCounter(&last);

    rate = _SILENT_RATE;

    auto due = LARGE_INTEGER{ .QuadPart = -static_cast<LONGLONG>(_SILENT_PERIOD_MS) * 10000 };
    ::SetWaitableTimer(timer, &due, _SILENT_PERIOD_MS, nullptr, nullptr, FALSE);

    HANDLE const handles[] = { timer, quit, deviceChanged };
    auto pending = 0.0;

    for (;;) {
        auto const signaled = ::WaitForMultipleObjects(3, handles, FALSE, INFINITE);

        if (signaled != WAIT_OBJECT_0) {
            break;
        }

        /* the frames that real time has played since the last turn */
        auto now = LARGE_INTEGER{};
        ::QueryPerformanceCounter(&now);

        pending += (static_cast<double>(now.QuadPart - last.QuadPart) / static_cast<double>(frequency.QuadPart)) * _SILENT_RATE;
        last = now;

        auto const frames = static_cast<std::uint32_t>(pending);
        pending -= frames;

        if (frames > 0) {
            scratch.resize(static_cast<std::size_t>(frames) * 2);
            mix(scratch.data(), frames, _SILENT_RATE);
        }
    }

    ::CloseHandle(timer);
}

void _Mixer::run() {
    PROFILE

    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    auto task = DWORD(0);
    auto const priority = ::AvSetMmThreadCharacteristicsW(L"Games", &task);

    auto enumerator = ComPtr<IMMDeviceEnumerator>();
    auto notifications = _DeviceNotifications(deviceChanged);

    if (SUCCEEDED(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) {
        enumerator->RegisterEndpointNotificationCallback(&notifications);
    }

    while (::WaitForSingleObject(quit, 0) != WAIT_OBJECT_0) {
        ::ResetEvent(deviceChanged);

        if ((enumerator == nullptr) || (render(enumerator.Get()) == false)) {
            renderSilently();
        }
    }

    if (enumerator != nullptr) {
        enumerator->UnregisterEndpointNotificationCallback(&notifications);
    }

    if (priority != nullptr) {
        ::AvRevertMmThreadCharacteristics(priority);
    }

    ::CoUninitialize();
}

static _Mixer& _GetMixer() {
    PROFILE

    if (_mixer == nullptr) {
        _mixer = new _Mixer{};
        _mixer->quit = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        _mixer->deviceChanged = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        _mixer->thread = std::thread([] { _mixer->run(); });

        /* stopped before the process tears down what the thread uses */
        std::atexit([] {
            ::SetEvent(_mixer->quit);
            _mixer->thread.join();
        });
    }

    return *_mixer;
}

static void _StartMediaFoundation() {
    PROFILE

    static auto const _started = [] {
        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        return SUCCEEDED(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
    }();

    if (_started == false) {
        throw std::runtime_error("Failed to start Media Foundation");
    }
}

static std::uint32_t _ReadBigEndian(std::uint8_t const* bytes, int count) {
    PROFILE

    auto value = std::uint32_t(0);

    for (auto i = 0; i < count; i++) {
        value = (value << 8) | bytes[i];
    }

    return value;
}

/* The 80-bit extended float AIFF keeps its sample rate in. */
static double _ReadExtended(std::uint8_t const* bytes) {
    PROFILE

    auto const exponent = static_cast<int>(_ReadBigEndian(bytes, 2) & 0x7FFF);
    auto const mantissa = (static_cast<std::uint64_t>(_ReadBigEndian(bytes + 2, 4)) << 32) | _ReadBigEndian(bytes + 6, 4);

    if ((exponent == 0) && (mantissa == 0)) {
        return 0.0;
    }

    return std::ldexp(static_cast<double>(mantissa), (exponent - 16383 - 63));
}

static bool _IsAiff(std::vector<std::uint8_t> const& bytes) {
    PROFILE

    return (bytes.size() >= 12)
        && (std::memcmp(bytes.data(), "FORM", 4) == 0)
        && ((std::memcmp(bytes.data() + 8, "AIFF", 4) == 0) || (std::memcmp(bytes.data() + 8, "AIFC", 4) == 0));
}

/* AIFF and AIFC with uncompressed samples ('NONE', 'sowt', 'fl32', 'fl64'),
   which Media Foundation does not read. */
static void _DecodeAiff(_DecodedAudio& audio, std::vector<std::uint8_t> const& bytes, std::string const& name) {
    PROFILE

    auto const fail = [&]() { throw std::runtime_error("Failed to decode audio: " + name); };
    auto const compressed = (std::memcmp(bytes.data() + 8, "AIFC", 4) == 0);

    auto channelCount = 0;
    auto frameCount = std::int64_t(0);
    auto bits = 0;
    auto rate = 0.0;
    auto little = false;
    auto floating = false;
    auto data = static_cast<std::uint8_t const*>(nullptr);
    auto dataSize = std::size_t(0);

    for (auto offset = std::size_t(12); (offset + 8) <= bytes.size(); ) {
        auto const chunk = bytes.data() + offset;
        auto const size = static_cast<std::size_t>(_ReadBigEndian(chunk + 4, 4));
        auto const body = chunk + 8;

        if ((offset + 8 + size) > bytes.size()) {
            fail();
        }

        if (std::memcmp(chunk, "COMM", 4) == 0) {
            if (size < 18) {
                fail();
            }

            channelCount = static_cast<int>(_ReadBigEndian(body, 2));
            frameCount = static_cast<std::int64_t>(_ReadBigEndian(body + 2, 4));
            bits = static_cast<int>(_ReadBigEndian(body + 6, 2));
            rate = _ReadExtended(body + 8);

            if (compressed && (size >= 22)) {
                auto const type = std::string(reinterpret_cast<char const*>(body + 18), 4);

                if ((type == "sowt")) {
                    little = true;
                } else if ((type == "fl32") || (type == "FL32")) {
                    floating = true;
                    bits = 32;
                } else if ((type == "fl64") || (type == "FL64")) {
                    floating = true;
                    bits = 64;
                } else if (type != "NONE") {
                    fail(); /* a compressed AIFC */
                }
            }
        } else if (std::memcmp(chunk, "SSND", 4) == 0) {
            if (size < 8) {
                fail();
            }

            auto const skip = static_cast<std::size_t>(_ReadBigEndian(body, 4));

            data = body + 8 + skip;
            dataSize = (size - 8 - std::min(skip, (size - 8)));
        }

        offset += 8 + size + (size & 1); /* chunks are padded to an even size */
    }

    auto const bytesPerSample = ((bits + 7) / 8);

    if ((channelCount <= 0) || (rate <= 0.0) || (data == nullptr) || (bytesPerSample <= 0) || (bytesPerSample > 8)) {
        fail();
    }

    frameCount = std::min(frameCount, static_cast<std::int64_t>(dataSize / (static_cast<std::size_t>(bytesPerSample) * channelCount)));

    audio.rate = static_cast<std::int32_t>(std::lround(rate));
    audio.channelCount = channelCount;
    audio.frameCount = frameCount;
    audio.channels.assign(static_cast<std::size_t>(channelCount), std::vector<float>(static_cast<std::size_t>(frameCount)));

    for (auto frame = std::int64_t(0); frame < frameCount; frame++) {
        for (auto channel = 0; channel < channelCount; channel++) {
            auto const sample = data + (((frame * channelCount) + channel) * bytesPerSample);
            auto value = std::uint64_t(0);

            for (auto i = 0; i < bytesPerSample; i++) {
                auto const byte = little ? sample[bytesPerSample - 1 - i] : sample[i];
                value = (value << 8) | byte;
            }

            auto output = 0.0f;

            if (floating && (bytesPerSample == 4)) {
                auto const word = static_cast<std::uint32_t>(value);
                auto number = 0.0f;
                std::memcpy(&number, &word, 4);
                output = number;
            } else if (floating) {
                auto number = 0.0;
                std::memcpy(&number, &value, 8);
                output = static_cast<float>(number);
            } else {
                /* signed, left-aligned in its bytes */
                auto const width = (bytesPerSample * 8);
                auto const signedValue = static_cast<std::int64_t>(value << (64 - width)) >> (64 - width);
                output = static_cast<float>(static_cast<double>(signedValue) / static_cast<double>(std::int64_t(1) << (width - 1)));
            }

            audio.channels[static_cast<std::size_t>(channel)][static_cast<std::size_t>(frame)] = output;
        }
    }
}

/* What a byte stream holds, from its first bytes: Media Foundation picks
   its reader by the content type, as a stream has no file name to go by. */
static std::wstring _GetContentType(std::vector<std::uint8_t> const& bytes) {
    PROFILE

    auto const starts = [&](std::size_t at, char const* text) {
        auto const length = std::strlen(text);
        return ((at + length) <= bytes.size()) && (std::memcmp(bytes.data() + at, text, length) == 0);
    };

    if (starts(0, "RIFF") && starts(8, "WAVE")) {
        return L"audio/wav";
    }

    if (starts(4, "ftyp")) {
        return L"audio/mp4";
    }

    if (starts(0, "fLaC")) {
        return L"audio/flac";
    }

    if (starts(0, "ID3") || ((bytes.size() >= 2) && (bytes[0] == 0xFF) && ((bytes[1] & 0xE0) == 0xE0))) {
        return L"audio/mpeg";
    }

    return L"";
}

static void _DecodeMediaFoundation(_DecodedAudio& audio, std::vector<std::uint8_t> const& bytes, std::string const& name) {
    PROFILE

    _StartMediaFoundation();

    auto const loadFailed = [&]() { return std::runtime_error("Failed to load audio: " + name); };
    auto const decodeFailed = [&]() { return std::runtime_error("Failed to decode audio: " + name); };

    auto const memory = ComPtr<IStream>(::SHCreateMemStream(bytes.data(), static_cast<UINT>(bytes.size())));
    auto stream = ComPtr<IMFByteStream>();
    auto reader = ComPtr<IMFSourceReader>();

    if ((memory == nullptr) || FAILED(::MFCreateMFByteStreamOnStream(memory.Get(), &stream))) {
        throw loadFailed();
    }

    auto const contentType = _GetContentType(bytes);
    auto attributes = ComPtr<IMFAttributes>();

    if ((contentType.empty() == false) && SUCCEEDED(stream.As(&attributes))) {
        attributes->SetString(MF_BYTESTREAM_CONTENT_TYPE, contentType.c_str());
    }

    if (FAILED(::MFCreateSourceReaderFromByteStream(stream.Get(), nullptr, &reader))) {
        throw loadFailed();
    }

    auto constexpr streamIndex = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);

    reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    reader->SetStreamSelection(streamIndex, TRUE);

    /* float32 at the clip's own rate; 16-bit PCM where a decoder offers no float */
    auto const request = [&](GUID const& subtype) {
        auto type = ComPtr<IMFMediaType>();

        return SUCCEEDED(::MFCreateMediaType(&type))
            && SUCCEEDED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))
            && SUCCEEDED(type->SetGUID(MF_MT_SUBTYPE, subtype))
            && SUCCEEDED(reader->SetCurrentMediaType(streamIndex, nullptr, type.Get()));
    };

    auto const floating = request(MFAudioFormat_Float);

    if ((floating == false) && (request(MFAudioFormat_PCM) == false)) {
        throw decodeFailed();
    }

    auto current = ComPtr<IMFMediaType>();
    auto channelCount = UINT32(0);
    auto rate = UINT32(0);
    auto bits = UINT32(0);

    if (
        FAILED(reader->GetCurrentMediaType(streamIndex, &current)) ||
        FAILED(current->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channelCount)) ||
        FAILED(current->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)) ||
        (channelCount == 0) || (rate == 0)
    ) {
        throw decodeFailed();
    }

    current->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits);

    if ((floating == false) && (bits != 16)) {
        throw decodeFailed();
    }

    auto interleaved = std::vector<float>();

    for (;;) {
        auto flags = DWORD(0);
        auto sample = ComPtr<IMFSample>();

        if (FAILED(reader->ReadSample(streamIndex, 0, nullptr, &flags, nullptr, &sample))) {
            throw decodeFailed();
        }

        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
            break;
        }

        if (sample == nullptr) {
            continue;
        }

        auto buffer = ComPtr<IMFMediaBuffer>();
        auto data = static_cast<BYTE*>(nullptr);
        auto length = DWORD(0);

        if (FAILED(sample->ConvertToContiguousBuffer(&buffer)) || FAILED(buffer->Lock(&data, nullptr, &length))) {
            throw decodeFailed();
        }

        if (floating) {
            auto const values = reinterpret_cast<float const*>(data);
            interleaved.insert(interleaved.end(), values, (values + (length / sizeof(float))));
        } else {
            auto const values = reinterpret_cast<std::int16_t const*>(data);

            for (auto i = DWORD(0); i < (length / sizeof(std::int16_t)); i++) {
                interleaved.push_back(static_cast<float>(values[i]) / 32768.0f);
            }
        }

        buffer->Unlock();
    }

    audio.rate = static_cast<std::int32_t>(rate);
    audio.channelCount = static_cast<std::int32_t>(channelCount);
    audio.frameCount = static_cast<std::int64_t>(interleaved.size() / channelCount);
    audio.channels.assign(channelCount, std::vector<float>(static_cast<std::size_t>(audio.frameCount)));

    for (auto frame = std::int64_t(0); frame < audio.frameCount; frame++) {
        for (auto channel = UINT32(0); channel < channelCount; channel++) {
            audio.channels[channel][static_cast<std::size_t>(frame)] = interleaved[static_cast<std::size_t>((frame * channelCount) + channel)];
        }
    }
}

static void _Decode(_DecodedAudio& audio, std::vector<std::uint8_t> const& bytes, std::string const& name) {
    PROFILE

    if (_IsAiff(bytes)) {
        _DecodeAiff(audio, bytes, name);
    } else {
        _DecodeMediaFoundation(audio, bytes, name);
    }

    if ((audio.channelCount <= 0) || (audio.frameCount <= 0)) {
        throw std::runtime_error("Failed to decode audio: " + name);
    }
}

/* One past the end starts over. Called with the mixer locked. */
static float _ScheduleFrom(_Voice& voice, bool repeat, float from) {
    PROFILE

    voice.activeDecay = std::min(voice.decayFrames, voice.rangeFrames);
    voice.activeFrames = (voice.activeDecay > 0) ? voice.activeDecay : voice.rangeFrames;
    voice.loop = repeat;

    auto startFrame = static_cast<std::int64_t>(from * static_cast<float>(voice.clip->rate));

    if (startFrame >= voice.activeFrames) {
        startFrame = 0;
    }

    voice.cursor = static_cast<double>(startFrame);
    voice.playing = (voice.activeFrames > 0);
    voice.stopped = false;

    return (static_cast<float>(startFrame) / static_cast<float>(voice.clip->rate));
}

void Audio::__init(std::string const& path) {
    PROFILE

    auto file = std::ifstream(std::filesystem::path(reinterpret_cast<char8_t const*>(path.c_str())), std::ios::binary);

    if (file.is_open() == false) {
        throw std::runtime_error("Failed to load audio: " + path);
    }

    auto const bytes = std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    _impl = new _Audio{};

    try {
        _Decode(*_impl, bytes, path);
    } catch (...) {
        delete _impl;
        _impl = nullptr;
        throw;
    }
}

void Audio::__init(std::uint8_t const* data, std::size_t size) {
    PROFILE

    _impl = new _Audio{};

    try {
        _Decode(*_impl, std::vector<std::uint8_t>(data, (data + size)), "audio data");
    } catch (...) {
        delete _impl;
        _impl = nullptr;
        throw;
    }
}

void Audio::__done() {
    PROFILE

    delete _impl;
    _impl = nullptr;
}

float Audio::__getDuration() const {
    PROFILE

    return (static_cast<float>(_impl->frameCount) / static_cast<float>(_impl->rate));
}

float Audio::__getSampleRate() const {
    PROFILE

    return static_cast<float>(_impl->rate);
}

std::int32_t Audio::__getChannelCount() const {
    PROFILE

    return _impl->channelCount;
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
    auto const rate = static_cast<float>(clip.rate);
    auto const rangeStart = std::min(static_cast<std::int64_t>(start * rate), clip.frameCount);
    auto const rangeFrames = std::min(static_cast<std::int64_t>(length * rate), (clip.frameCount - rangeStart));

    _impl = new _Sound{};
    _impl->clip = &clip;
    _impl->rangeStart = rangeStart;
    _impl->rangeFrames = rangeFrames;
    _impl->activeFrames = rangeFrames;

    auto& mixer = _GetMixer();
    auto lock = std::scoped_lock(mixer.mutex);

    mixer.voices.push_back(_impl);
}

void Sound::__done() {
    PROFILE

    if (_impl != nullptr) {
        auto& mixer = _GetMixer();

        {
            auto lock = std::scoped_lock(mixer.mutex);
            std::erase(mixer.voices, static_cast<_Voice*>(_impl));
        }

        delete _impl;
        _impl = nullptr;
    }
}

void Sound::__play() {
    PROFILE

    {
        auto lock = std::scoped_lock(_GetMixer().mutex);
        _ScheduleFrom(*_impl, _loop, 0.0f);
    }

    _paused = false;
    _impl->position = 0.0f;
}

void Sound::__pause() {
    PROFILE

    auto const position = __getPosition();

    {
        auto lock = std::scoped_lock(_GetMixer().mutex);
        _impl->playing = false;
    }

    _impl->position = position;
    _paused = true;
}

void Sound::__resume() {
    PROFILE

    /* scheduled again, as on macOS: the loop and decay set since play() apply */
    {
        auto lock = std::scoped_lock(_GetMixer().mutex);
        _ScheduleFrom(*_impl, _loop, _impl->position);
    }

    _paused = false;
}

void Sound::__stop() {
    PROFILE

    {
        auto lock = std::scoped_lock(_GetMixer().mutex);
        _impl->playing = false;
        _impl->stopped = true;
    }

    _paused = false;
    _impl->position = 0.0f;
}

void Sound::__setVolume(float volume) {
    PROFILE

    auto lock = std::scoped_lock(_GetMixer().mutex);

    _impl->volume = volume;
}

void Sound::__setPan(float pan) {
    PROFILE

    auto lock = std::scoped_lock(_GetMixer().mutex);

    /* A balance: the side panned away from fades out, the other stays at
       full gain, so the center leaves the sound as it is. */
    _impl->left = std::min(1.0f, (1.0f - pan));
    _impl->right = std::min(1.0f, (1.0f + pan));
}

void Sound::__setPitch(float pitch) {
    PROFILE

    auto lock = std::scoped_lock(_GetMixer().mutex);

    /* the frequency ratio shifts speed and pitch together, as macOS's varispeed does */
    _impl->ratio = std::clamp(std::pow(2.0f, (pitch / 12.0f)), 0.01f, 100.0f);
}

void Sound::__setDecay(float decay) {
    PROFILE

    auto lock = std::scoped_lock(_GetMixer().mutex);

    /* taken up by the next play or resume, as on the other backends */
    _impl->decayFrames = (decay > 0.0f)
        ? std::max(static_cast<std::int64_t>(decay * static_cast<float>(_impl->clip->rate)), std::int64_t(1))
        : 0;
}

void Sound::__setPosition(float position) {
    PROFILE

    auto const playing = __isPlaying();

    /* a sound that is not playing starts paused at the position: the mixer is
       locked so that none of it is mixed in between */
    {
        auto lock = std::scoped_lock(_GetMixer().mutex);

        _impl->position = _ScheduleFrom(*_impl, _loop, position);

        if (playing == false) {
            _impl->playing = false;
        }
    }

    if (playing == false) {
        _paused = true;
    }
}

float Sound::__getPosition() const {
    PROFILE

    if (_paused) {
        return _impl->position;
    }

    auto lock = std::scoped_lock(_GetMixer().mutex);

    if (_impl->stopped) {
        return 0.0f;
    }

    /* the cursor counts frames of the clip itself, before the frequency ratio */
    auto const frames = std::clamp(_impl->cursor, 0.0, static_cast<double>(_impl->activeFrames));

    return static_cast<float>(frames / static_cast<double>(_impl->clip->rate));
}

bool Sound::__isPlaying() const {
    PROFILE

    if (_paused) {
        return false;
    }

    auto lock = std::scoped_lock(_GetMixer().mutex);

    return _impl->playing;
}

} /* namespace Rocket */
