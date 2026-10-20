/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <cstring>
#include <windows.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/String.hpp>
#include <Rocket/Window/App_Win32.hpp>
#include <Rocket/Window/Clipboard_Private.hpp>

namespace Rocket {

static auto constexpr _OPEN_ATTEMPTS = 10;
static auto constexpr _OPEN_RETRY_DELAY_MS = 5;

static HWND _GetOwner() {
    PROFILE

    static auto const _owner = [] {
        App::__InitWin32();

        return ::CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, ::GetModuleHandleW(nullptr), nullptr);
    }();

    return _owner;
}

static bool _OpenClipboard() {
    PROFILE

    for (auto attempt = 0; attempt < _OPEN_ATTEMPTS; attempt++) {
        if (::OpenClipboard(_GetOwner())) {
            return true;
        }

        ::Sleep(_OPEN_RETRY_DELAY_MS);
    }

    return false;
}

std::string __GetClipboardString() {
    PROFILE

    if (_OpenClipboard() == false) {
        return "";
    }

    auto string = std::string();
    auto const data = ::GetClipboardData(CF_UNICODETEXT);

    if (data != nullptr) {
        auto const text = static_cast<wchar_t const*>(::GlobalLock(data));

        if (text != nullptr) {
            string = App::__FromWide(text);
            ::GlobalUnlock(data);
        }
    }

    ::CloseClipboard();

    return StringReplace(string, "\r\n", "\n");
}

void __SetClipboardString(std::string const& string) {
    PROFILE

    auto const wide = App::__ToWide(StringReplace(StringReplace(string, "\r\n", "\n"), "\n", "\r\n"));
    auto const bytes = (wide.size() + 1) * sizeof(wchar_t);
    auto const data = ::GlobalAlloc(GMEM_MOVEABLE, bytes);

    if (data == nullptr) {
        return;
    }

    auto const text = static_cast<wchar_t*>(::GlobalLock(data));
    std::memcpy(text, wide.c_str(), bytes);
    ::GlobalUnlock(data);

    if (_OpenClipboard() == false) {
        ::GlobalFree(data);
        return;
    }

    ::EmptyClipboard();

    if (::SetClipboardData(CF_UNICODETEXT, data) == nullptr) {
        ::GlobalFree(data);
    }

    ::CloseClipboard();
}

} /* namespace Rocket */
