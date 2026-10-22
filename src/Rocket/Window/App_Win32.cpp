/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <cmath>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Private.hpp>
#include <Rocket/Window/Gamepad_Private.hpp>
#include <Rocket/Window/App_Win32.hpp>
#include <Rocket/Window/Window_Win32.hpp>

namespace Rocket {
namespace App {

static auto constexpr _DEFAULT_REFRESH_RATE = 60.0;

static auto _isExited = false;
static auto _frameEnd = std::uint64_t(0);
static auto _timerPainting = 0;
static auto _timerId = UINT_PTR(0);

static double _GetFramePeriod() {
    PROFILE

    auto mode = DEVMODEW{ .dmSize = sizeof(DEVMODEW) };

    auto const rate = (::EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && (mode.dmDisplayFrequency > 1))
        ? static_cast<double>(mode.dmDisplayFrequency)
        : _DEFAULT_REFRESH_RATE;

    return 1.0 / rate;
}

static std::uint64_t _GetTicks() {
    PROFILE

    auto counter = LARGE_INTEGER{};
    ::QueryPerformanceCounter(&counter);

    return static_cast<std::uint64_t>(counter.QuadPart);
}

static std::uint64_t _GetTicksPerSecond() {
    PROFILE

    auto frequency = LARGE_INTEGER{};
    ::QueryPerformanceFrequency(&frequency);

    return static_cast<std::uint64_t>(frequency.QuadPart);
}

static void CALLBACK _OnPaintTimer(HWND, UINT, UINT_PTR, DWORD) {
    PROFILE

    __PaintWindows();
}

static bool _IsCoalescedMessage(MSG const& message) {
    PROFILE

    return (message.message == WM_MOUSEMOVE)
        || (message.message == WM_MOUSEWHEEL)
        || (message.message == WM_MOUSEHWHEEL);
}

static void _CoalesceMessage(std::vector<MSG>& held, MSG const& message) {
    PROFILE

    auto constexpr buttons = WPARAM(MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2);

    if (held.empty() == false) {
        auto& last = held.back();

        if (
            (message.message == WM_MOUSEMOVE) &&
            (last.message == WM_MOUSEMOVE) &&
            (last.hwnd == message.hwnd) &&
            ((last.wParam & buttons) == (message.wParam & buttons))
        ) {
            last = message;
            return;
        }

        if (
            (message.message == last.message) &&
            ((message.message == WM_MOUSEWHEEL) || (message.message == WM_MOUSEHWHEEL)) &&
            (last.hwnd == message.hwnd)
        ) {
            auto const delta = std::clamp(GET_WHEEL_DELTA_WPARAM(last.wParam) + GET_WHEEL_DELTA_WPARAM(message.wParam), -32768, 32767);
            last.wParam = MAKEWPARAM(LOWORD(message.wParam), static_cast<WORD>(static_cast<SHORT>(delta)));
            last.lParam = message.lParam;
            return;
        }
    }

    held.push_back(message);
}

static void _DispatchMessage(MSG const& message) {
    PROFILE

    if (message.message == WM_QUIT) {
        _Close();
        return;
    }

    ::TranslateMessage(&message);
    ::DispatchMessageW(&message);
}

static void _DispatchHeldMessages(std::vector<MSG>& held) {
    PROFILE

    auto const messages = std::move(held);
    held.clear();

    for (auto const& message : messages) {
        _DispatchMessage(message);
    }
}

static bool _DrainMessages(std::vector<MSG>& held) {
    PROFILE

    auto message = MSG{};
    auto handled = false;

    while (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (_IsCoalescedMessage(message)) {
            _CoalesceMessage(held, message);
        } else {
            _DispatchHeldMessages(held);
            _DispatchMessage(message);
            handled = true;
        }
    }

    return handled;
}

static std::string _TrimSeparator(std::string path) {
    PROFILE

    while ((path.size() > 3) && ((path.back() == '\\') || (path.back() == '/'))) {
        path.pop_back();
    }

    return path;
}

void __InitWin32() {
    PROFILE

    static thread_local auto _comInitialized = false;
    static auto _initialized = false;

    if (_comInitialized == false) {
        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        _comInitialized = true;
    }

    if (_initialized) {
        return;
    }

    ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    _initialized = true;
}

std::wstring __ToWide(std::string_view string) {
    PROFILE

    if (string.empty()) {
        return {};
    }

    auto const size = ::MultiByteToWideChar(CP_UTF8, 0, string.data(), static_cast<int>(string.size()), nullptr, 0);
    auto wide = std::wstring(static_cast<std::size_t>(size), L'\0');

    ::MultiByteToWideChar(CP_UTF8, 0, string.data(), static_cast<int>(string.size()), wide.data(), size);

    return wide;
}

std::string __FromWide(std::wstring_view string) {
    PROFILE

    if (string.empty()) {
        return {};
    }

    auto const size = ::WideCharToMultiByte(CP_UTF8, 0, string.data(), static_cast<int>(string.size()), nullptr, 0, nullptr, nullptr);
    auto narrow = std::string(static_cast<std::size_t>(size), '\0');

    ::WideCharToMultiByte(CP_UTF8, 0, string.data(), static_cast<int>(string.size()), narrow.data(), size, nullptr, nullptr);

    return narrow;
}

std::string const& __GetExecPath() {
    PROFILE

    static auto const _execPath = []{
        auto path = std::wstring(MAX_PATH, L'\0');

        for (;;) {
            auto const length = ::GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));

            if (length == 0) {
                return std::string();
            }

            if (length < path.size()) {
                path.resize(length);
                return __FromWide(path);
            }

            path.resize(path.size() * 2);
        }
    }();

    return _execPath;
}

std::string const& __GetExecDir() {
    PROFILE

    static auto const _execDir = [] {
        auto const& path = __GetExecPath();
        auto const separator = path.find_last_of("\\/");

        return (separator != std::string::npos) ? _TrimSeparator(path.substr(0, separator + 1)) : std::string();
    }();

    return _execDir;
}

std::string const& __GetWorkDir() {
    PROFILE

    static auto _workDir = std::string();

    auto const length = ::GetCurrentDirectoryW(0, nullptr);
    auto directory = std::wstring(length, L'\0');
    auto const written = ::GetCurrentDirectoryW(length, directory.data());

    directory.resize(written);
    _workDir = _TrimSeparator(__FromWide(directory));

    return _workDir;
}

std::string const& __GetUserDir() {
    PROFILE

    static auto const _userDir = [] {
        auto path = PWSTR(nullptr);
        auto folder = std::string();

        if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &path))) {
            folder = _TrimSeparator(__FromWide(path));
        }

        ::CoTaskMemFree(path);

        return folder;
    }();

    return _userDir;
}

void __Exit() {
    PROFILE

    _isExited = true;
}

void __Run(int argc, char const** argv) {
    // PROFILE

    __InitWin32();

    _isExited = false;

    while (_isExited == false) {
        __Tick(true);
    }
}

void __Tick(bool update) {
    PROFILE

    __InitWin32();

    static auto const _timer = ::CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    static thread_local auto _held = std::vector<MSG>();

    for (;;) {
        auto const now = _GetTicks();

        if (now >= _frameEnd) {
            _DrainMessages(_held);
            break;
        }

        auto const remaining = static_cast<double>(_frameEnd - now) / static_cast<double>(_GetTicksPerSecond());
        auto due = LARGE_INTEGER{ .QuadPart = -static_cast<LONGLONG>(remaining * 10000000.0) };

        ::SetWaitableTimer(_timer, &due, 0, nullptr, nullptr, FALSE);

        auto const result = ::MsgWaitForMultipleObjectsEx(1, &_timer, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

        if (result == WAIT_OBJECT_0) {
            _DrainMessages(_held);
            break;
        }

        if (_DrainMessages(_held)) {
            break;
        }
    }

    _DispatchHeldMessages(_held);
    __UpdateGamepads();

    if (update) {
        _Update();
    }

    if (_GetTicks() >= _frameEnd) {
        /* From the frame's deadline, not from when the loop woke, so a late wake
           does not add to every frame. A loop more than a frame behind starts a
           new beat instead of catching up. */
        _frameEnd += static_cast<std::uint64_t>(_GetFramePeriod() * static_cast<double>(_GetTicksPerSecond()));

        if (_frameEnd <= _GetTicks()) {
            _frameEnd = _GetTicks() + static_cast<std::uint64_t>(_GetFramePeriod() * static_cast<double>(_GetTicksPerSecond()));
        }

        __PaintWindows();
    }
}

void __SetTimerPainting(bool painting) {
    PROFILE

    _timerPainting += painting ? 1 : -1;

    if ((_timerPainting > 0) && (_timerId == 0)) {
        _timerId = ::SetTimer(nullptr, 0, static_cast<UINT>(std::max(1.0, std::floor(_GetFramePeriod() * 1000.0))), _OnPaintTimer);
    } else if ((_timerPainting == 0) && (_timerId != 0)) {
        ::KillTimer(nullptr, _timerId);
        _timerId = 0;
    }
}

} /* namespace App */
} /* namespace Rocket */
