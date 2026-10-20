/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <map>
#include <cmath>
#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <windows.h>
#include <objbase.h>
#include <windowsx.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <imm.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/String.hpp>
#include <Rocket/Paint/Painter_Win32.hpp>
#include <Rocket/Window/App_Private.hpp>
#include <Rocket/Window/App_Win32.hpp>
#include <Rocket/Window/Input_Private.hpp>
#include <Rocket/Window/Window_Win32.hpp>

namespace Rocket {

struct Window::_Window {
    HWND hwnd;
    Vec2 size;
    Vec2 position;
    float scale;
    bool created;
    bool visible;
    bool closable;
    bool sizable;
    bool minimizable;
    bool maximizable;
    bool maximized;
    bool minimized;
    bool fullscreen;
    bool hover;
    bool cursorHidden;
    bool composing;
    bool modal;
    bool resizing;
    std::optional<Vec2> mousePosition;
    std::vector<Mouse> pressedButtons;
    std::optional<Mouse> clickButton;
    LONG clickTime;
    POINT clickPoint;
    int clickCount;
    wchar_t highSurrogate;
    LONG_PTR windowedStyle;
    WINDOWPLACEMENT windowedPlacement;
};

static auto constexpr _WHEEL_SCALE = 50.0f;
static auto constexpr _WHEEL_DELTA = 120.0f;
static auto constexpr _DEFAULT_WIDTH = 640.0f;
static auto constexpr _DEFAULT_HEIGHT = 480.0f;
static auto constexpr _WM_ROCKET_PAINT = static_cast<UINT>(WM_APP + 1);
static auto constexpr _WINDOW_CLASS = L"RocketWindow";

static std::map<std::uint32_t, Scancode> const _keyMap = {
    { 0x0001, Scancode::Escape },
    { 0x0002, Scancode::Digit1 },
    { 0x0003, Scancode::Digit2 },
    { 0x0004, Scancode::Digit3 },
    { 0x0005, Scancode::Digit4 },
    { 0x0006, Scancode::Digit5 },
    { 0x0007, Scancode::Digit6 },
    { 0x0008, Scancode::Digit7 },
    { 0x0009, Scancode::Digit8 },
    { 0x000A, Scancode::Digit9 },
    { 0x000B, Scancode::Digit0 },
    { 0x000C, Scancode::Minus },
    { 0x000D, Scancode::Equal },
    { 0x000E, Scancode::Backspace },
    { 0x000F, Scancode::Tab },
    { 0x0010, Scancode::KeyQ },
    { 0x0011, Scancode::KeyW },
    { 0x0012, Scancode::KeyE },
    { 0x0013, Scancode::KeyR },
    { 0x0014, Scancode::KeyT },
    { 0x0015, Scancode::KeyY },
    { 0x0016, Scancode::KeyU },
    { 0x0017, Scancode::KeyI },
    { 0x0018, Scancode::KeyO },
    { 0x0019, Scancode::KeyP },
    { 0x001A, Scancode::BracketLeft },
    { 0x001B, Scancode::BracketRight },
    { 0x001C, Scancode::Enter },
    { 0x001D, Scancode::Control },
    { 0x001E, Scancode::KeyA },
    { 0x001F, Scancode::KeyS },
    { 0x0020, Scancode::KeyD },
    { 0x0021, Scancode::KeyF },
    { 0x0022, Scancode::KeyG },
    { 0x0023, Scancode::KeyH },
    { 0x0024, Scancode::KeyJ },
    { 0x0025, Scancode::KeyK },
    { 0x0026, Scancode::KeyL },
    { 0x0027, Scancode::Semicolon },
    { 0x0028, Scancode::Quote },
    { 0x0029, Scancode::Backquote },
    { 0x002A, Scancode::Shift },
    { 0x002B, Scancode::Backslash },
    { 0x002C, Scancode::KeyZ },
    { 0x002D, Scancode::KeyX },
    { 0x002E, Scancode::KeyC },
    { 0x002F, Scancode::KeyV },
    { 0x0030, Scancode::KeyB },
    { 0x0031, Scancode::KeyN },
    { 0x0032, Scancode::KeyM },
    { 0x0033, Scancode::Comma },
    { 0x0034, Scancode::Period },
    { 0x0035, Scancode::Slash },
    { 0x0036, Scancode::ShiftRight },
    { 0x0037, Scancode::NumpadMultiply },
    { 0x0038, Scancode::Alt },
    { 0x0039, Scancode::Space },
    { 0x003A, Scancode::Capslock },
    { 0x003B, Scancode::F1 },
    { 0x003C, Scancode::F2 },
    { 0x003D, Scancode::F3 },
    { 0x003E, Scancode::F4 },
    { 0x003F, Scancode::F5 },
    { 0x0040, Scancode::F6 },
    { 0x0041, Scancode::F7 },
    { 0x0042, Scancode::F8 },
    { 0x0043, Scancode::F9 },
    { 0x0044, Scancode::F10 },
    { 0x0045, Scancode::Pause },     /* Windows swaps Pause and NumLock: Pause comes without the extended flag */
    { 0x0046, Scancode::ScrollLock },
    { 0x0047, Scancode::Numpad7 },
    { 0x0048, Scancode::Numpad8 },
    { 0x0049, Scancode::Numpad9 },
    { 0x004A, Scancode::NumpadMinus },
    { 0x004B, Scancode::Numpad4 },
    { 0x004C, Scancode::Numpad5 },
    { 0x004D, Scancode::Numpad6 },
    { 0x004E, Scancode::NumpadAdd },
    { 0x004F, Scancode::Numpad1 },
    { 0x0050, Scancode::Numpad2 },
    { 0x0051, Scancode::Numpad3 },
    { 0x0052, Scancode::Numpad0 },
    { 0x0053, Scancode::NumpadDecimal },
    { 0x0054, Scancode::PrintScreen }, /* Alt+PrintScreen comes as SysRq */
    { 0x0056, Scancode::IntlBackslash },
    { 0x0057, Scancode::F11 },
    { 0x0058, Scancode::F12 },
    { 0x0059, Scancode::NumpadEqual },
    { 0x0064, Scancode::F13 },
    { 0x0065, Scancode::F14 },
    { 0x0066, Scancode::F15 },
    { 0x0067, Scancode::F16 },
    { 0x0068, Scancode::F17 },
    { 0x0069, Scancode::F18 },
    { 0x006A, Scancode::F19 },
    { 0x006B, Scancode::F20 },
    { 0xE01C, Scancode::NumpadEnter },
    { 0xE01D, Scancode::ControlRight },
    { 0xE020, Scancode::Mute },
    { 0xE02E, Scancode::VolumeDown },
    { 0xE030, Scancode::VolumeUp },
    { 0xE035, Scancode::NumpadDivide },
    { 0xE037, Scancode::PrintScreen },
    { 0xE038, Scancode::AltRight },
    { 0xE045, Scancode::NumLock },
    { 0xE046, Scancode::Pause },     /* Control+Pause comes as Break */
    { 0xE047, Scancode::Home },
    { 0xE048, Scancode::ArrowUp },
    { 0xE049, Scancode::PageUp },
    { 0xE04B, Scancode::ArrowLeft },
    { 0xE04D, Scancode::ArrowRight },
    { 0xE04F, Scancode::End },
    { 0xE050, Scancode::ArrowDown },
    { 0xE051, Scancode::PageDown },
    { 0xE052, Scancode::Insert },
    { 0xE053, Scancode::Delete },
    { 0xE05B, Scancode::Meta },
    { 0xE05C, Scancode::MetaRight },
    { 0xE05D, Scancode::ContextMenu }
};

static Scancode _KeyConvert(std::uint32_t native) {
    PROFILE

    auto const it = _keyMap.find(native);

    return (it != _keyMap.end()) ? it->second : Scancode::Unknown;
}

static std::uint32_t _GetNativeScancode(WPARAM wParam, LPARAM lParam) {
    PROFILE

    auto scancode = static_cast<std::uint32_t>((lParam >> 16) & 0xFF);

    if (scancode == 0) {
        auto const mapped = ::MapVirtualKeyW(static_cast<UINT>(wParam), MAPVK_VK_TO_VSC_EX);
        return ((mapped & 0xFF00) != 0) ? (0xE000 | (mapped & 0xFF)) : mapped;
    }

    if ((lParam & (1 << 24)) != 0) {
        scancode |= 0xE000;
    }

    return scancode;
}

static KeyModifiers _GetModifiers() {
    PROFILE

    auto const down = [](int key) { return (::GetKeyState(key) & 0x8000) != 0; };

    return {
        .control = down(VK_CONTROL),
        .shift = down(VK_SHIFT),
        .meta = (down(VK_LWIN) || down(VK_RWIN)),
        .alt = down(VK_MENU)
    };
}

static bool _IsControlCodepoint(std::uint32_t codepoint) {
    PROFILE

    return (codepoint < 0x20u) || ((codepoint >= 0x7Fu) && (codepoint < 0xA0u));
}

static std::uint32_t _JoinSurrogates(wchar_t high, wchar_t low) {
    PROFILE

    return 0x10000u + ((static_cast<std::uint32_t>(high) - 0xD800u) << 10) + (static_cast<std::uint32_t>(low) - 0xDC00u);
}

static std::vector<std::uint32_t> _ToCodepoints(std::wstring_view text) {
    PROFILE

    auto codepoints = std::vector<std::uint32_t>();

    for (auto i = std::size_t(0); i < text.size(); i++) {
        auto const unit = text[i];

        if ((unit >= 0xD800) && (unit < 0xDC00) && ((i + 1) < text.size())) {
            codepoints.push_back(_JoinSurrogates(unit, text[i + 1]));
            i++;
        } else {
            codepoints.push_back(unit);
        }
    }

    return codepoints;
}

static std::int32_t _CountCodepoints(std::wstring_view text, std::size_t units) {
    PROFILE

    return static_cast<std::int32_t>(_ToCodepoints(text.substr(0, std::min(units, text.size()))).size());
}

static std::string _SanitizeInput(std::wstring_view text) {
    PROFILE

    auto kept = std::vector<std::uint32_t>();

    for (auto const codepoint : _ToCodepoints(text)) {
        if (_IsControlCodepoint(codepoint) == false) {
            kept.push_back(codepoint);
        }
    }

    auto string = std::string();

    StringFromCodepoints(kept, string);

    return string;
}

static std::string _GetKeycode(std::uint32_t native, Scancode scancode) {
    PROFILE

    if (__IsCharacterScancode(scancode) == false) {
        return GetDefaultKeycode(scancode);
    }

    auto buffer = std::array<wchar_t, 8>{};
    auto const state = std::array<BYTE, 256>{};
    auto const layout = ::GetKeyboardLayout(0);
    auto const virtualKey = ::MapVirtualKeyExW(native, MAPVK_VSC_TO_VK_EX, layout);
    auto const length = ::ToUnicodeEx(virtualKey, native & 0xFF, state.data(), buffer.data(), static_cast<int>(buffer.size()), 0x4, layout);

    if (length <= 0) {
        return GetDefaultKeycode(scancode);
    }

    auto const codepoints = _ToCodepoints(std::wstring_view(buffer.data(), static_cast<std::size_t>(length)));

    if ((codepoints.size() != 1) || _IsControlCodepoint(codepoints.front())) {
        return GetDefaultKeycode(scancode);
    }

    auto keycode = std::string();
    StringFromCodepoints({ CodepointToLower(codepoints.front()) }, keycode);

    return keycode;
}

std::string __GetLayoutKeycode(Scancode scancode) {
    PROFILE

    App::__InitWin32();

    for (auto const& [ native, mapped ] : _keyMap) {
        if (mapped == scancode) {
            return _GetKeycode(native, scancode);
        }
    }

    return GetDefaultKeycode(scancode);
}

static LPCWSTR _CursorConvert(Cursor cursor) {
    PROFILE

    switch (cursor) {
        case Cursor::Default:      return IDC_ARROW;
        case Cursor::ContextMenu:  return IDC_ARROW;       // NOTE: Not supported in Win32
        case Cursor::Help:         return IDC_HELP;
        case Cursor::Pointer:      return IDC_HAND;
        case Cursor::Progress:     return IDC_APPSTARTING;
        case Cursor::Wait:         return IDC_WAIT;
        case Cursor::Cell:         return IDC_CROSS;       // NOTE: Not supported in Win32
        case Cursor::Crosshair:    return IDC_CROSS;
        case Cursor::Text:         return IDC_IBEAM;
        case Cursor::VerticalText: return IDC_IBEAM;       // NOTE: Not supported in Win32
        case Cursor::Alias:        return IDC_ARROW;       // NOTE: Not supported in Win32
        case Cursor::Copy:         return IDC_ARROW;       // NOTE: Not supported in Win32
        case Cursor::Move:         return IDC_SIZEALL;
        case Cursor::NoDrop:       return IDC_NO;
        case Cursor::NotAllowed:   return IDC_NO;
        case Cursor::Grab:         return IDC_HAND;        // NOTE: Not supported in Win32
        case Cursor::Grabbing:     return IDC_HAND;        // NOTE: Not supported in Win32
        case Cursor::EResize:      return IDC_SIZEWE;
        case Cursor::NResize:      return IDC_SIZENS;
        case Cursor::NEResize:     return IDC_SIZENESW;
        case Cursor::NWResize:     return IDC_SIZENWSE;
        case Cursor::SResize:      return IDC_SIZENS;
        case Cursor::SEResize:     return IDC_SIZENWSE;
        case Cursor::SWResize:     return IDC_SIZENESW;
        case Cursor::WResize:      return IDC_SIZEWE;
        case Cursor::EWResize:     return IDC_SIZEWE;
        case Cursor::NSResize:     return IDC_SIZENS;
        case Cursor::NESWResize:   return IDC_SIZENESW;
        case Cursor::ColResize:    return IDC_SIZEWE;      // NOTE: Not supported in Win32
        case Cursor::RowResize:    return IDC_SIZENS;      // NOTE: Not supported in Win32
        case Cursor::AllScroll:    return IDC_SIZEALL;     // NOTE: Not supported in Win32
        case Cursor::ZoomIn:       return IDC_ARROW;       // NOTE: Not supported in Win32
        case Cursor::ZoomOut:      return IDC_ARROW;       // NOTE: Not supported in Win32
        case Cursor::None:         return IDC_ARROW;       /* hidden instead, see __setCursor */
    }

    return IDC_ARROW;
}

static HCURSOR _GetCursor(Cursor cursor) {
    PROFILE

    return (cursor == Cursor::None) ? nullptr : ::LoadCursorW(nullptr, _CursorConvert(cursor));
}

static LONG_PTR _GetStyle(LONG_PTR current, bool fullscreen, bool sizable, bool maximizable, bool minimizable) {
    PROFILE

    auto style = current & (WS_VISIBLE | WS_MAXIMIZE | WS_MINIMIZE | WS_DISABLED);

    if (fullscreen) {
        return style | WS_POPUP;
    }

    style |= (WS_CAPTION | WS_SYSMENU);

    if (sizable) {
        style |= WS_THICKFRAME;
    }

    if (maximizable) {
        style |= WS_MAXIMIZEBOX;
    }

    if (minimizable) {
        style |= WS_MINIMIZEBOX;
    }

    return style;
}

static RECT _PositionMonitorRect(HWND hwnd, bool visible) {
    PROFILE

    auto monitor = visible ? ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL) : nullptr;

    if (monitor == nullptr) {
        monitor = ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    }

    auto info = MONITORINFO{ .cbSize = sizeof(MONITORINFO) };
    ::GetMonitorInfoW(monitor, &info);

    return info.rcMonitor;
}

static RECT _GetFrameInsets(HWND hwnd) {
    PROFILE

    auto window = RECT{};
    auto frame = RECT{};

    ::GetWindowRect(hwnd, &window);

    if (SUCCEEDED(::DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame)))) {
        return RECT{
            frame.left - window.left,
            frame.top - window.top,
            window.right - frame.right,
            window.bottom - frame.bottom
        };
    }

    auto const style = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
    auto const exStyle = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    auto adjusted = RECT{ 0, 0, 0, 0 };

    ::AdjustWindowRectExForDpi(&adjusted, style, FALSE, exStyle, ::GetDpiForWindow(hwnd));

    if ((style & WS_THICKFRAME) == 0 && (style & WS_CAPTION) == 0) {
        return RECT{ 0, 0, 0, 0 };
    }

    return RECT{
        std::max(0L, -adjusted.left - 1),
        0,
        std::max(0L, adjusted.right - 1),
        std::max(0L, adjusted.bottom - 1)
    };
}

static SIZE _GetWindowSize(HWND hwnd, int width, int height) {
    PROFILE

    auto const style = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
    auto const exStyle = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    auto rect = RECT{ 0, 0, width, height };

    ::AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, ::GetDpiForWindow(hwnd));

    return SIZE{ rect.right - rect.left, rect.bottom - rect.top };
}

static void _UpdateCloseButton(HWND hwnd, bool closable) {
    PROFILE

    auto const menu = ::GetSystemMenu(hwnd, FALSE);

    if (menu != nullptr) {
        ::EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | (closable ? MF_ENABLED : MF_GRAYED));
    }
}

static void _ApplyStyle(HWND hwnd, LONG_PTR style, bool closable) {
    PROFILE

    auto client = RECT{};
    ::GetClientRect(hwnd, &client);

    ::SetWindowLongPtrW(hwnd, GWL_STYLE, style);

    auto const outer = _GetWindowSize(hwnd, client.right, client.bottom);
    auto const resizable = ((style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0);

    ::SetWindowPos(
        hwnd, nullptr, 0, 0, outer.cx, outer.cy,
        (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | (resizable ? 0 : SWP_NOSIZE))
    );

    _UpdateCloseButton(hwnd, closable);
}

void __PaintWindows() {
    PROFILE

    auto const windows = Window::GetWindows();

    for (auto const window : windows) {
        if (std::ranges::find(Window::GetWindows(), window) == Window::GetWindows().end()) {
            continue;
        }

        auto const hwnd = static_cast<HWND>(window->getHandle());

        if (window->getVisible() && (::IsIconic(hwnd) == FALSE)) {
            ::SendMessageW(hwnd, _WM_ROCKET_PAINT, 0, 0);
        }
    }
}

void Window::__done() {
    PROFILE

    if (_impl->cursorHidden) {
        ::ShowCursor(TRUE);
    }

    if (_impl->modal) {
        _impl->modal = false;
    }

    ::SetWindowLongPtrW(_impl->hwnd, GWLP_USERDATA, 0);

    __ReleaseWindowTarget(_impl->hwnd);
    ::DestroyWindow(_impl->hwnd);

    delete _impl;
}

void Window::__init() {
    PROFILE

    App::__InitWin32();

    static auto const _registered = [] {
        auto const windowClass = WNDCLASSEXW{
            .cbSize = sizeof(WNDCLASSEXW),
            .style = 0,
            .lpfnWndProc = reinterpret_cast<WNDPROC>(&Window::__WindowProc),
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = ::GetModuleHandleW(nullptr),
            .hIcon = ::LoadIconW(nullptr, IDI_APPLICATION),
            .hCursor = nullptr,
            .hbrBackground = nullptr,
            .lpszMenuName = nullptr,
            .lpszClassName = _WINDOW_CLASS,
            .hIconSm = nullptr
        };

        return (::RegisterClassExW(&windowClass) != 0);
    }();

    if (_registered == false) {
        throw std::runtime_error("Failed to register the window class");
    }

    _impl = new Window::_Window{
        .hwnd = nullptr,
        .size = { _DEFAULT_WIDTH, _DEFAULT_HEIGHT },
        .position = { 0.0f, 0.0f },
        .scale = 1.0f,
        .created = false,
        .visible = false,
        .closable = false,
        .sizable = false,
        .minimizable = false,
        .maximizable = true,
        .maximized = false,
        .minimized = false,
        .fullscreen = false,
        .hover = false,
        .cursorHidden = false,
        .composing = false,
        .modal = false,
        .resizing = false,
        .mousePosition = std::nullopt,
        .pressedButtons = {},
        .clickButton = std::nullopt,
        .clickTime = 0,
        .clickPoint = POINT{ 0, 0 },
        .clickCount = 0,
        .highSurrogate = 0,
        .windowedStyle = 0,
        .windowedPlacement = WINDOWPLACEMENT{ .length = sizeof(WINDOWPLACEMENT) }
    };

    auto const style = static_cast<DWORD>(_GetStyle(0, false, _impl->sizable, _impl->maximizable, _impl->minimizable));

    _impl->hwnd = ::CreateWindowExW(
        0, _WINDOW_CLASS, L"", style,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, ::GetModuleHandleW(nullptr), this
    );

    if (_impl->hwnd == nullptr) {
        delete _impl;
        throw std::runtime_error("Failed to create a window");
    }

    _impl->scale = static_cast<float>(::GetDpiForWindow(_impl->hwnd)) / 96.0f;

    auto const outer = _GetWindowSize(
        _impl->hwnd,
        static_cast<int>(std::lround(_DEFAULT_WIDTH * _impl->scale)),
        static_cast<int>(std::lround(_DEFAULT_HEIGHT * _impl->scale))
    );

    ::SetWindowPos(_impl->hwnd, nullptr, 0, 0, outer.cx, outer.cy, (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));

    _UpdateCloseButton(_impl->hwnd, _impl->closable);

    ::ImmAssociateContextEx(_impl->hwnd, nullptr, 0);
    ::DragAcceptFiles(_impl->hwnd, TRUE);

    _impl->created = true;
}

std::intptr_t __stdcall Window::__WindowProc(void* handle, std::uint32_t message, std::uintptr_t wParam, std::intptr_t lParam) {
    PROFILE

    auto const hwnd = static_cast<HWND>(handle);

    if (message == WM_NCCREATE) {
        auto const create = reinterpret_cast<CREATESTRUCTW const*>(lParam);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    auto const window = reinterpret_cast<Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if ((window == nullptr) || (window->_impl == nullptr) || (window->_impl->hwnd != hwnd) || (window->_impl->created == false)) {
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    auto& impl = *window->_impl;

    auto const toPoints = [&](int x, int y) {
        return Vec2{ (static_cast<float>(x) / impl.scale), (static_cast<float>(y) / impl.scale) };
    };

    auto const cursorPosition = [&]() {
        auto point = POINT{};
        ::GetCursorPos(&point);
        ::ScreenToClient(hwnd, &point);
        return toPoints(point.x, point.y);
    };

    auto const dispatchMouseMove = [&](Vec2 const& position) {
        impl.mousePosition = position;
        window->_mouseMove(position, _GetModifiers());
    };

    auto const releaseButtons = [&](Vec2 const& position) {
        auto const pressed = impl.pressedButtons;
        impl.pressedButtons.clear();
        for (auto const mouse : pressed) {
            window->_mouseUp(mouse, position, _GetModifiers());
        }
    };

    auto const updateSize = [&]() {
        auto client = RECT{};
        ::GetClientRect(hwnd, &client);

        if ((::IsIconic(hwnd) == FALSE) && (client.right > 0) && (client.bottom > 0)) {
            auto const size = toPoints(client.right, client.bottom);
            auto const changed = (size != impl.size);
            impl.size = size;
            return changed;
        }

        return false;
    };

    auto const mouseDown = [&](Mouse mouse) {
        auto const x = GET_X_LPARAM(lParam);
        auto const y = GET_Y_LPARAM(lParam);
        auto const position = toPoints(x, y);

        if (impl.mousePosition != position) {
            dispatchMouseMove(position);
        }

        auto const time = ::GetMessageTime();
        auto const sameClick =
            (impl.clickButton == mouse) &&
            (static_cast<UINT>(time - impl.clickTime) <= ::GetDoubleClickTime()) &&
            (std::abs(x - impl.clickPoint.x) <= (::GetSystemMetrics(SM_CXDOUBLECLK) / 2)) &&
            (std::abs(y - impl.clickPoint.y) <= (::GetSystemMetrics(SM_CYDOUBLECLK) / 2));

        impl.clickCount = sameClick ? (impl.clickCount + 1) : 1;
        impl.clickButton = mouse;
        impl.clickTime = time;
        impl.clickPoint = POINT{ x, y };

        if (impl.pressedButtons.empty()) {
            ::SetCapture(hwnd);
        }

        if (std::ranges::find(impl.pressedButtons, mouse) == impl.pressedButtons.end()) {
            impl.pressedButtons.push_back(mouse);
        }

        window->_mouseDown(mouse, position, _GetModifiers(), impl.clickCount);
    };

    auto const mouseUp = [&](Mouse mouse) {
        auto const position = toPoints(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

        std::erase(impl.pressedButtons, mouse);

        if (impl.pressedButtons.empty() && (::GetCapture() == hwnd)) {
            ::ReleaseCapture();
        }

        window->_mouseUp(mouse, position, _GetModifiers());
    };

    auto const keyDown = [&](std::uint32_t native) {
        auto const scancode = _KeyConvert(native);
        auto const repeat = ((lParam & (1 << 30)) != 0);

        window->_keyDown(scancode, _GetKeycode(native, scancode), _GetModifiers(), repeat);
    };

    auto const keyUp = [&](std::uint32_t native) {
        auto const scancode = _KeyConvert(native);

        window->_keyUp(scancode, _GetKeycode(native, scancode), _GetModifiers());
    };

    switch (message) {
        case WM_CLOSE: {
            window->_close();
            return 0;
        }
        case WM_SYSCOMMAND: {
            auto const command = (wParam & 0xFFF0);

            if ((command == SC_CLOSE) && (impl.closable == false)) {
                return 0;
            }

            if ((command == SC_KEYMENU) && (lParam != VK_SPACE)) {
                return 0;
            }

            break;
        }
        case WM_QUERYENDSESSION: {
            App::_Close();
            return TRUE;
        }
        case WM_ERASEBKGND: {
            return 1;
        }
        case WM_PAINT: {
            auto paint = PAINTSTRUCT{};
            ::BeginPaint(hwnd, &paint);
            ::EndPaint(hwnd, &paint);
            return 0;
        }
        case _WM_ROCKET_PAINT: {
            window->_paint();
            return 0;
        }
        case WM_ENTERSIZEMOVE:
        case WM_ENTERMENULOOP: {
            App::__SetTimerPainting(true);
            break;
        }
        case WM_EXITSIZEMOVE:
        case WM_EXITMENULOOP: {
            App::__SetTimerPainting(false);
            break;
        }
        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED) {
                if (impl.minimized == false) {
                    impl.minimized = true;
                    window->_minimize();
                }
                return 0;
            }

            if (impl.minimized) {
                impl.minimized = false;
                window->_deminimize();
            } else if ((wParam == SIZE_MAXIMIZED) && (impl.maximized == false)) {
                impl.maximized = true;
                window->_maximize();
            } else if ((wParam == SIZE_RESTORED) && impl.maximized) {
                impl.maximized = false;
                window->_demaximize();
            }

            if (updateSize() && (impl.resizing == false)) {
                window->_resize();
            }
            return 0;
        }
        case WM_DPICHANGED: {
            impl.scale = static_cast<float>(HIWORD(wParam)) / 96.0f;

            auto const suggested = reinterpret_cast<RECT const*>(lParam);

            impl.resizing = true;
            ::SetWindowPos(
                hwnd, nullptr,
                suggested->left, suggested->top,
                (suggested->right - suggested->left), (suggested->bottom - suggested->top),
                (SWP_NOZORDER | SWP_NOACTIVATE)
            );
            impl.resizing = false;

            updateSize();
            window->_dpiChange();
            window->_resize();
            return 0;
        }
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                ::SetCursor(_GetCursor(window->getCursor()));
                return TRUE;
            }
            break;
        }
        case WM_SETFOCUS: {
            window->_focus();

            if (impl.hover) {
                dispatchMouseMove(cursorPosition());
            }
            return 0;
        }
        case WM_KILLFOCUS: {
            window->_blur();
            return 0;
        }
        case WM_MOUSEMOVE: {
            auto const position = toPoints(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

            if (impl.hover == false) {
                auto track = TRACKMOUSEEVENT{ .cbSize = sizeof(TRACKMOUSEEVENT), .dwFlags = TME_LEAVE, .hwndTrack = hwnd };
                ::TrackMouseEvent(&track);

                impl.hover = true;
                window->_mouseEnter(position, _GetModifiers());
            }

            if (((wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) == 0) && (impl.pressedButtons.empty() == false)) {
                releaseButtons(position);
            }

            dispatchMouseMove(position);
            return 0;
        }
        case WM_MOUSELEAVE: {
            impl.hover = false;
            window->_mouseExit(cursorPosition(), _GetModifiers());
            return 0;
        }
        case WM_CAPTURECHANGED: {
            if ((reinterpret_cast<HWND>(lParam) != hwnd) && (impl.pressedButtons.empty() == false)) {
                releaseButtons(impl.mousePosition.value_or(cursorPosition()));
            }
            return 0;
        }
        case WM_LBUTTONDOWN: { mouseDown(Mouse::LeftButton); return 0; }
        case WM_RBUTTONDOWN: { mouseDown(Mouse::RightButton); return 0; }
        case WM_MBUTTONDOWN: { mouseDown(Mouse::MiddleButton); return 0; }
        case WM_LBUTTONUP:   { mouseUp(Mouse::LeftButton); return 0; }
        case WM_RBUTTONUP:   { mouseUp(Mouse::RightButton); return 0; }
        case WM_MBUTTONUP:   { mouseUp(Mouse::MiddleButton); return 0; }
        case WM_MOUSEWHEEL: {
            auto const notches = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / _WHEEL_DELTA;

            window->_mouseWheel({ 0.0f, (notches * _WHEEL_SCALE) }, _GetModifiers());
            return 0;
        }
        case WM_MOUSEHWHEEL: {
            auto const notches = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / _WHEEL_DELTA;

            window->_mouseWheel({ (notches * -_WHEEL_SCALE), 0.0f }, _GetModifiers());
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (impl.composing || (wParam == VK_PROCESSKEY)) {
                break;
            }

            if ((wParam == VK_CONTROL) && ((lParam & (1 << 24)) == 0)) {
                auto next = MSG{};
                auto const time = ::GetMessageTime();

                if (
                    ::PeekMessageW(&next, hwnd, WM_KEYDOWN, WM_SYSKEYDOWN, PM_NOREMOVE) &&
                    ((next.message == WM_KEYDOWN) || (next.message == WM_SYSKEYDOWN)) &&
                    (next.wParam == VK_MENU) &&
                    ((next.lParam & (1 << 24)) != 0) &&
                    (static_cast<LONG>(next.time) == time)
                ) {
                    return 0;
                }
            }

            keyDown(_GetNativeScancode(wParam, lParam));

            if ((message == WM_SYSKEYDOWN) && (wParam == VK_F4)) {
                break;
            }
            return 0;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            auto const native = _GetNativeScancode(wParam, lParam);
            auto const scancode = _KeyConvert(native);

            if (
                (scancode == Scancode::PrintScreen) &&
                (std::ranges::find(window->_pressedKeys, scancode, &std::pair<Scancode, std::string>::first) == window->_pressedKeys.end())
            ) {
                keyDown(native);
            }

            keyUp(native);
            return 0;
        }
        case WM_SYSCHAR: {
            return 0;
        }
        case WM_CHAR: {
            auto const unit = static_cast<wchar_t>(wParam);

            if ((unit >= 0xD800) && (unit < 0xDC00)) {
                impl.highSurrogate = unit;
                return 0;
            }

            auto text = std::wstring();

            if ((unit >= 0xDC00) && (unit < 0xE000)) {
                if (impl.highSurrogate != 0) {
                    text = { impl.highSurrogate, unit };
                }
            } else {
                text = { unit };
            }

            impl.highSurrogate = 0;

            if (window->getInputArea().has_value() == false) {
                return 0;
            }

            auto const input = _SanitizeInput(text);

            if (input.empty() == false) {
                window->_input(input);
            }
            return 0;
        }
        case WM_IME_SETCONTEXT: {
            return ::DefWindowProcW(hwnd, message, wParam, (lParam & ~static_cast<LPARAM>(ISC_SHOWUICOMPOSITIONWINDOW)));
        }
        case WM_IME_STARTCOMPOSITION: {
            return 0;
        }
        case WM_IME_COMPOSITION: {
            auto const context = ::ImmGetContext(hwnd);

            if (context == nullptr) {
                return 0;
            }

            auto const read = [&](DWORD index) {
                auto const bytes = ::ImmGetCompositionStringW(context, index, nullptr, 0);
                auto text = std::wstring((bytes > 0) ? (static_cast<std::size_t>(bytes) / sizeof(wchar_t)) : 0, L'\0');

                if (text.empty() == false) {
                    ::ImmGetCompositionStringW(context, index, text.data(), static_cast<DWORD>(bytes));
                }
                return text;
            };

            auto result = std::optional<std::wstring>();
            auto composition = std::optional<std::tuple<std::wstring, std::int32_t, std::int32_t>>();

            if ((lParam & GCS_RESULTSTR) != 0) {
                result = read(GCS_RESULTSTR);
            }

            if ((lParam & GCS_COMPSTR) != 0) {
                auto const text = read(GCS_COMPSTR);
                auto const cursorUnits = ::ImmGetCompositionStringW(context, GCS_CURSORPOS, nullptr, 0);

                auto attributes = std::vector<BYTE>(text.size());
                auto selectionStart = text.size();
                auto selectionEnd = text.size();

                if (attributes.empty() == false) {
                    ::ImmGetCompositionStringW(context, GCS_COMPATTR, attributes.data(), static_cast<DWORD>(attributes.size()));

                    for (auto i = std::size_t(0); i < attributes.size(); i++) {
                        if ((attributes[i] == ATTR_TARGET_CONVERTED) || (attributes[i] == ATTR_TARGET_NOTCONVERTED)) {
                            selectionStart = std::min(selectionStart, i);
                            selectionEnd = i + 1;
                        }
                    }
                }

                auto const hasSelection = (selectionStart < text.size());
                auto const cursor = hasSelection
                    ? _CountCodepoints(text, selectionStart)
                    : _CountCodepoints(text, static_cast<std::size_t>(std::max(cursorUnits, 0L)));
                auto const length = hasSelection
                    ? (_CountCodepoints(text, selectionEnd) - _CountCodepoints(text, selectionStart))
                    : 0;

                composition = std::make_tuple(text, cursor, length);
            }

            ::ImmReleaseContext(hwnd, context);

            if (result.has_value()) {
                if (impl.composing) {
                    impl.composing = false;
                    window->_composition("", 0, 0);
                }

                auto const input = _SanitizeInput(*result);

                if (input.empty() == false) {
                    window->_input(input);
                }
            }

            if (composition.has_value()) {
                auto const& [ text, cursor, length ] = *composition;

                if ((text.empty() == false) || impl.composing) {
                    impl.composing = (text.empty() == false);
                    window->_composition(App::__FromWide(text), cursor, length);
                }
            }

            return 0;
        }
        case WM_IME_ENDCOMPOSITION: {
            if (impl.composing) {
                impl.composing = false;
                window->_composition("", 0, 0);
            }
            return 0;
        }
        case WM_DROPFILES: {
            auto const drop = reinterpret_cast<HDROP>(wParam);
            auto const count = ::DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
            auto paths = std::vector<std::string>();

            for (auto i = UINT(0); i < count; i++) {
                auto path = std::wstring(::DragQueryFileW(drop, i, nullptr, 0), L'\0');
                ::DragQueryFileW(drop, i, path.data(), static_cast<UINT>(path.size() + 1));
                paths.push_back(App::__FromWide(path));
            }

            auto point = POINT{};
            ::DragQueryPoint(drop, &point);
            ::DragFinish(drop);

            if (paths.empty() == false) {
                window->_dropFiles(paths, toPoints(point.x, point.y));
            }
            return 0;
        }
        default: {
            break;
        }
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

Vec2 const& Window::__getPosition() const {
    PROFILE

    auto const monitor = _PositionMonitorRect(_impl->hwnd, _impl->visible);
    auto const insets = _GetFrameInsets(_impl->hwnd);
    auto rect = RECT{};

    ::GetWindowRect(_impl->hwnd, &rect);

    _impl->position = {
        (static_cast<float>((rect.left + insets.left) - monitor.left) / _impl->scale),
        (static_cast<float>((rect.top + insets.top) - monitor.top) / _impl->scale)
    };

    return _impl->position;
}

Vec2 const& Window::__getSize() const {
    PROFILE

    return _impl->size;
}

float Window::__getScale() const {
    PROFILE

    return _impl->scale;
}

void* Window::__getHandle() const {
    PROFILE

    return _impl->hwnd;
}

bool Window::__getVisible() const {
    PROFILE

    return _impl->visible;
}

bool Window::__getClosable() const {
    PROFILE

    return _impl->closable;
}

bool Window::__getSizable() const {
    PROFILE

    return _impl->sizable;
}

bool Window::__getMaximizable() const {
    PROFILE

    return _impl->maximizable;
}

bool Window::__getMinimizable() const {
    PROFILE

    return _impl->minimizable;
}

bool Window::__getTopmost() const {
    PROFILE

    return ((::GetWindowLongPtrW(_impl->hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);
}

bool Window::__isMaximized() const {
    PROFILE

    return (::IsZoomed(_impl->hwnd) != FALSE);
}

bool Window::__isMinimized() const {
    PROFILE

    return (::IsIconic(_impl->hwnd) != FALSE);
}

bool Window::__isFullscreen() const {
    PROFILE

    return _impl->fullscreen;
}

bool Window::__isHover() const {
    PROFILE

    return _impl->hover;
}

bool Window::__isFocused() const {
    PROFILE

    return (::GetFocus() == _impl->hwnd);
}

void Window::__center() const {
    PROFILE

    auto const monitor = ::MonitorFromWindow(_impl->hwnd, MONITOR_DEFAULTTOPRIMARY);
    auto info = MONITORINFO{ .cbSize = sizeof(MONITORINFO) };
    auto rect = RECT{};

    ::GetMonitorInfoW(monitor, &info);
    ::GetWindowRect(_impl->hwnd, &rect);

    auto const width = (rect.right - rect.left);
    auto const height = (rect.bottom - rect.top);
    auto const area = info.rcWork;

    ::SetWindowPos(
        _impl->hwnd, nullptr,
        (area.left + (((area.right - area.left) - width) / 2)),
        (area.top + (((area.bottom - area.top) - height) / 2)),
        0, 0,
        (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)
    );
}

void Window::__paint() const {
    PROFILE

    /* The run loop paints every window once a frame anyway (see App::__Tick). */
}

void Window::__setTitle(std::string const& title) {
    PROFILE

    ::SetWindowTextW(_impl->hwnd, App::__ToWide(title).c_str());
}

void Window::__setInputArea(std::optional<Vec4> const& caretRect) {
    PROFILE

    if (caretRect.has_value()) {
        ::ImmAssociateContextEx(_impl->hwnd, nullptr, IACE_DEFAULT);

        auto const context = ::ImmGetContext(_impl->hwnd);

        if (context != nullptr) {
            auto const scale = _impl->scale;
            auto const area = RECT{
                static_cast<LONG>(std::floor(caretRect->x * scale)),
                static_cast<LONG>(std::floor(caretRect->y * scale)),
                static_cast<LONG>(std::ceil((caretRect->x + caretRect->width) * scale)),
                static_cast<LONG>(std::ceil((caretRect->y + caretRect->height) * scale))
            };

            auto composition = COMPOSITIONFORM{ .dwStyle = CFS_POINT, .ptCurrentPos = POINT{ area.left, area.top }, .rcArea = area };
            auto candidate = CANDIDATEFORM{ .dwIndex = 0, .dwStyle = CFS_EXCLUDE, .ptCurrentPos = POINT{ area.left, area.bottom }, .rcArea = area };

            ::ImmSetCompositionWindow(context, &composition);
            ::ImmSetCandidateWindow(context, &candidate);
            ::ImmReleaseContext(_impl->hwnd, context);
        }
    } else {
        auto const context = ::ImmGetContext(_impl->hwnd);

        if (context != nullptr) {
            ::ImmNotifyIME(context, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            ::ImmReleaseContext(_impl->hwnd, context);
        }

        ::ImmAssociateContextEx(_impl->hwnd, nullptr, 0);

        if (_impl->composing) {
            _impl->composing = false;
            _composition("", 0, 0);
        }
    }
}

void Window::__setCursor(Cursor cursor) {
    PROFILE

    if (cursor == Cursor::None) {
        if (_impl->cursorHidden == false) {
            ::ShowCursor(FALSE);
            _impl->cursorHidden = true;
        }
    } else if (_impl->cursorHidden) {
        ::ShowCursor(TRUE);
        _impl->cursorHidden = false;
    }

    if (_impl->hover) {
        ::SetCursor(_GetCursor(cursor));
    }
}

void Window::__setPosition(Vec2 const& position) {
    PROFILE

    auto const monitor = _PositionMonitorRect(_impl->hwnd, _impl->visible);
    auto const insets = _GetFrameInsets(_impl->hwnd);

    ::SetWindowPos(
        _impl->hwnd, nullptr,
        ((monitor.left + static_cast<int>(std::lround(position.x * _impl->scale))) - insets.left),
        ((monitor.top + static_cast<int>(std::lround(position.y * _impl->scale))) - insets.top),
        0, 0,
        (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)
    );
}

void Window::__setSize(Vec2 const& size) {
    PROFILE

    auto const width = static_cast<int>(std::lround(size.width * _impl->scale));
    auto const height = static_cast<int>(std::lround(size.height * _impl->scale));
    auto const outer = _GetWindowSize(_impl->hwnd, width, height);

    ::SetWindowPos(_impl->hwnd, nullptr, 0, 0, outer.cx, outer.cy, (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));

    _impl->size = size;
}

void Window::__setVisible(bool visible) {
    PROFILE

    if (visible) {
        if (_impl->visible == false) {
            ::SetWindowPos(_impl->hwnd, HWND_TOP, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW));
            ::SetForegroundWindow(_impl->hwnd);
            _impl->visible = true;
            _show();
        }
    } else {
        if (_impl->visible) {
            ::SetWindowPos(_impl->hwnd, nullptr, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW));
            _impl->visible = false;
            _hide();
        }
    }
}

void Window::__setClosable(bool closable) {
    PROFILE

    _impl->closable = closable;
    _UpdateCloseButton(_impl->hwnd, closable);
}

void Window::__setSizable(bool sizable) {
    PROFILE

    _impl->sizable = sizable;

    if (_impl->fullscreen == false) {
        auto const current = ::GetWindowLongPtrW(_impl->hwnd, GWL_STYLE);
        _ApplyStyle(_impl->hwnd, _GetStyle(current, false, _impl->sizable, _impl->maximizable, _impl->minimizable), _impl->closable);
    }
}

void Window::__setMaximizable(bool maximizable) {
    PROFILE

    _impl->maximizable = maximizable;

    if (_impl->fullscreen == false) {
        auto const current = ::GetWindowLongPtrW(_impl->hwnd, GWL_STYLE);
        _ApplyStyle(_impl->hwnd, _GetStyle(current, false, _impl->sizable, _impl->maximizable, _impl->minimizable), _impl->closable);
    }
}

void Window::__setMinimizable(bool minimizable) {
    PROFILE

    _impl->minimizable = minimizable;

    if (_impl->fullscreen == false) {
        auto const current = ::GetWindowLongPtrW(_impl->hwnd, GWL_STYLE);
        _ApplyStyle(_impl->hwnd, _GetStyle(current, false, _impl->sizable, _impl->maximizable, _impl->minimizable), _impl->closable);
    }
}

void Window::__setMaximized(bool maximized) {
    PROFILE

    if (maximized != isMaximized()) {
        ::ShowWindow(_impl->hwnd, maximized ? SW_MAXIMIZE : SW_RESTORE);
        _impl->visible = (::IsWindowVisible(_impl->hwnd) != FALSE);
    }
}

void Window::__setMinimized(bool minimized) {
    PROFILE

    if (minimized != isMinimized()) {
        ::ShowWindow(_impl->hwnd, minimized ? SW_MINIMIZE : SW_RESTORE);
        _impl->visible = (::IsWindowVisible(_impl->hwnd) != FALSE);
    }
}

void Window::__setFullscreen(bool fullscreen) {
    PROFILE

    if (fullscreen == _impl->fullscreen) {
        return;
    }

    if (fullscreen) {
        _impl->windowedStyle = ::GetWindowLongPtrW(_impl->hwnd, GWL_STYLE);
        _impl->windowedPlacement = WINDOWPLACEMENT{ .length = sizeof(WINDOWPLACEMENT) };
        ::GetWindowPlacement(_impl->hwnd, &_impl->windowedPlacement);

        auto const monitor = ::MonitorFromWindow(_impl->hwnd, MONITOR_DEFAULTTOPRIMARY);
        auto info = MONITORINFO{ .cbSize = sizeof(MONITORINFO) };
        ::GetMonitorInfoW(monitor, &info);

        _impl->fullscreen = true;

        auto const style = _GetStyle(_impl->windowedStyle & ~static_cast<LONG_PTR>(WS_MAXIMIZE), true, false, false, false);
        ::SetWindowLongPtrW(_impl->hwnd, GWL_STYLE, style);
        ::SetWindowPos(
            _impl->hwnd, HWND_TOP,
            info.rcMonitor.left, info.rcMonitor.top,
            (info.rcMonitor.right - info.rcMonitor.left), (info.rcMonitor.bottom - info.rcMonitor.top),
            (SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOACTIVATE)
        );
    } else {
        _impl->fullscreen = false;

        auto const visible = ::GetWindowLongPtrW(_impl->hwnd, GWL_STYLE) & WS_VISIBLE;
        ::SetWindowLongPtrW(_impl->hwnd, GWL_STYLE, ((_impl->windowedStyle & ~static_cast<LONG_PTR>(WS_VISIBLE)) | visible));

        if (visible == 0) {
            _impl->windowedPlacement.showCmd = SW_HIDE;
        }

        ::SetWindowPlacement(_impl->hwnd, &_impl->windowedPlacement);
        ::SetWindowPos(_impl->hwnd, nullptr, 0, 0, 0, 0, (SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE));
        _UpdateCloseButton(_impl->hwnd, _impl->closable);
    }
}

void Window::__setTopmost(bool topmost) {
    PROFILE

    ::SetWindowPos(_impl->hwnd, (topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
}

void Window::__setFocus() {
    PROFILE

    if (_impl->visible == false) {
        ::SetWindowPos(_impl->hwnd, HWND_TOP, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW));
        _impl->visible = true;
    }

    ::SetForegroundWindow(_impl->hwnd);
    ::SetFocus(_impl->hwnd);
}

void Window::__runModal() {
    PROFILE

    auto const parent = GetFocusedWindow();

    auto disabled = std::vector<HWND>();

    for (auto const other : GetWindows()) {
        auto const hwnd = static_cast<HWND>(other->getHandle());

        if ((other != this) && ::IsWindowEnabled(hwnd)) {
            ::EnableWindow(hwnd, FALSE);
            disabled.push_back(hwnd);
        }
    }

    if ((parent != nullptr) && (parent != this)) {
        ::SetWindowLongPtrW(_impl->hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(parent->getHandle()));
    }

    _impl->modal = true;

    auto const self = this;

    while ((std::ranges::find(GetWindows(), self) != GetWindows().end()) && _impl->modal) {
        App::__Tick(false);
    }

    for (auto const hwnd : disabled) {
        if (::IsWindow(hwnd)) {
            ::EnableWindow(hwnd, TRUE);
        }
    }

    if ((std::ranges::find(GetWindows(), self) != GetWindows().end()) && (parent != nullptr) && (parent != this)) {
        ::SetWindowLongPtrW(_impl->hwnd, GWLP_HWNDPARENT, 0);
    }
}

void Window::__stopModal() {
    PROFILE

    _impl->modal = false;
}

} /* namespace Rocket */
