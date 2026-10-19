#include <map>
#include <cmath>
#include <string>
#include <vector>
#include <cassert>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/String.hpp>
#include <Rocket/Paint/Painter_Private.hpp>
#include <Rocket/Window/App_Private.hpp>
#include <Rocket/Window/Input_Private.hpp>
#include <Rocket/Window/Window_SDL3.hpp>

namespace Rocket {

/* SDL reports the wheel in notches (fractions of one for a touchpad); the
   macOS backend publishes points, and this many points make one notch */
static auto constexpr _WHEEL_SCALE = 50.0f;

/* The frame rate the windows are painted at when the display reports none */
static auto constexpr _DEFAULT_REFRESH_RATE = 60.0f;

/* How many times a new window is offered to the GPU device, and how long to wait between
   the tries: under load the Vulkan driver can fail to describe a new window's surface for
   a moment (seen with NVIDIA on Wayland, as an error SDL has no name for) */
static auto constexpr _CLAIM_ATTEMPTS = 5;
static auto constexpr _CLAIM_RETRY_DELAY_MS = 20;

/** Per-window native state: the SDL window and what SDL3 does not keep for us. */
struct Window::_Window {
    SDL_Window* window;
    Vec2 size;
    Vec2 position;
    float scale;
    bool visible;
    bool closable;
    bool minimizable;
    bool maximizable;
    bool maximized;
    bool minimized;
    bool hover;
    bool cursorHidden;
    bool composing;
    bool modal;
    std::optional<Vec2> hiddenSize;
    std::optional<Vec2> mousePosition;
    std::vector<Mouse> pressedButtons;
    std::vector<std::string> droppedFiles;
};

/** What the run loop calls on a window: set up by the window itself, which alone may reach its private members. */
struct _WindowHandlers {
    std::function<void(SDL_Event const&)> onEvent;
    std::function<void()> onPaint;
};

static auto _handlers = std::map<SDL_WindowID, _WindowHandlers>();
static auto _modalWindows = std::vector<SDL_WindowID>();
static auto _nextFrame = std::uint64_t(0);

static std::map<SDL_Scancode, Scancode> const _keyMap = {
    { SDL_SCANCODE_0, Scancode::Digit0 },
    { SDL_SCANCODE_1, Scancode::Digit1 },
    { SDL_SCANCODE_2, Scancode::Digit2 },
    { SDL_SCANCODE_3, Scancode::Digit3 },
    { SDL_SCANCODE_4, Scancode::Digit4 },
    { SDL_SCANCODE_5, Scancode::Digit5 },
    { SDL_SCANCODE_6, Scancode::Digit6 },
    { SDL_SCANCODE_7, Scancode::Digit7 },
    { SDL_SCANCODE_8, Scancode::Digit8 },
    { SDL_SCANCODE_9, Scancode::Digit9 },
    { SDL_SCANCODE_A, Scancode::KeyA },
    { SDL_SCANCODE_B, Scancode::KeyB },
    { SDL_SCANCODE_C, Scancode::KeyC },
    { SDL_SCANCODE_D, Scancode::KeyD },
    { SDL_SCANCODE_E, Scancode::KeyE },
    { SDL_SCANCODE_F, Scancode::KeyF },
    { SDL_SCANCODE_G, Scancode::KeyG },
    { SDL_SCANCODE_H, Scancode::KeyH },
    { SDL_SCANCODE_I, Scancode::KeyI },
    { SDL_SCANCODE_J, Scancode::KeyJ },
    { SDL_SCANCODE_K, Scancode::KeyK },
    { SDL_SCANCODE_L, Scancode::KeyL },
    { SDL_SCANCODE_M, Scancode::KeyM },
    { SDL_SCANCODE_N, Scancode::KeyN },
    { SDL_SCANCODE_O, Scancode::KeyO },
    { SDL_SCANCODE_P, Scancode::KeyP },
    { SDL_SCANCODE_Q, Scancode::KeyQ },
    { SDL_SCANCODE_R, Scancode::KeyR },
    { SDL_SCANCODE_S, Scancode::KeyS },
    { SDL_SCANCODE_T, Scancode::KeyT },
    { SDL_SCANCODE_U, Scancode::KeyU },
    { SDL_SCANCODE_V, Scancode::KeyV },
    { SDL_SCANCODE_W, Scancode::KeyW },
    { SDL_SCANCODE_X, Scancode::KeyX },
    { SDL_SCANCODE_Y, Scancode::KeyY },
    { SDL_SCANCODE_Z, Scancode::KeyZ },
    { SDL_SCANCODE_F1, Scancode::F1 },
    { SDL_SCANCODE_F2, Scancode::F2 },
    { SDL_SCANCODE_F3, Scancode::F3 },
    { SDL_SCANCODE_F4, Scancode::F4 },
    { SDL_SCANCODE_F5, Scancode::F5 },
    { SDL_SCANCODE_F6, Scancode::F6 },
    { SDL_SCANCODE_F7, Scancode::F7 },
    { SDL_SCANCODE_F8, Scancode::F8 },
    { SDL_SCANCODE_F9, Scancode::F9 },
    { SDL_SCANCODE_F10, Scancode::F10 },
    { SDL_SCANCODE_F11, Scancode::F11 },
    { SDL_SCANCODE_F12, Scancode::F12 },
    { SDL_SCANCODE_F13, Scancode::F13 },
    { SDL_SCANCODE_F14, Scancode::F14 },
    { SDL_SCANCODE_F15, Scancode::F15 },
    { SDL_SCANCODE_F16, Scancode::F16 },
    { SDL_SCANCODE_F17, Scancode::F17 },
    { SDL_SCANCODE_F18, Scancode::F18 },
    { SDL_SCANCODE_F19, Scancode::F19 },
    { SDL_SCANCODE_F20, Scancode::F20 },
    { SDL_SCANCODE_LALT, Scancode::Alt },
    { SDL_SCANCODE_DOWN, Scancode::ArrowDown },
    { SDL_SCANCODE_LEFT, Scancode::ArrowLeft },
    { SDL_SCANCODE_RIGHT, Scancode::ArrowRight },
    { SDL_SCANCODE_UP, Scancode::ArrowUp },
    { SDL_SCANCODE_BACKSLASH, Scancode::Backslash },
    { SDL_SCANCODE_NONUSHASH, Scancode::Backslash },
    { SDL_SCANCODE_BACKSPACE, Scancode::Backspace },
    { SDL_SCANCODE_LEFTBRACKET, Scancode::BracketLeft },
    { SDL_SCANCODE_RIGHTBRACKET, Scancode::BracketRight },
    { SDL_SCANCODE_CAPSLOCK, Scancode::Capslock },
    { SDL_SCANCODE_COMMA, Scancode::Comma },
    { SDL_SCANCODE_LCTRL, Scancode::Control },
    { SDL_SCANCODE_END, Scancode::End },
    { SDL_SCANCODE_EQUALS, Scancode::Equal },
    { SDL_SCANCODE_ESCAPE, Scancode::Escape },
    { SDL_SCANCODE_DELETE, Scancode::Delete },
    { SDL_SCANCODE_GRAVE, Scancode::Backquote },
    { SDL_SCANCODE_HELP, Scancode::Help },
    { SDL_SCANCODE_HOME, Scancode::Home },
    { SDL_SCANCODE_LGUI, Scancode::Meta },
    { SDL_SCANCODE_MINUS, Scancode::Minus },
    { SDL_SCANCODE_MUTE, Scancode::Mute },
    { SDL_SCANCODE_PAGEDOWN, Scancode::PageDown },
    { SDL_SCANCODE_PAGEUP, Scancode::PageUp },
    { SDL_SCANCODE_PERIOD, Scancode::Period },
    { SDL_SCANCODE_APOSTROPHE, Scancode::Quote },
    { SDL_SCANCODE_RETURN, Scancode::Enter },
    { SDL_SCANCODE_RALT, Scancode::AltRight },
    { SDL_SCANCODE_RCTRL, Scancode::ControlRight },
    { SDL_SCANCODE_RSHIFT, Scancode::ShiftRight },
    { SDL_SCANCODE_SEMICOLON, Scancode::Semicolon },
    { SDL_SCANCODE_LSHIFT, Scancode::Shift },
    { SDL_SCANCODE_SLASH, Scancode::Slash },
    { SDL_SCANCODE_SPACE, Scancode::Space },
    { SDL_SCANCODE_TAB, Scancode::Tab },
    { SDL_SCANCODE_VOLUMEDOWN, Scancode::VolumeDown },
    { SDL_SCANCODE_VOLUMEUP, Scancode::VolumeUp },
    { SDL_SCANCODE_KP_CLEAR, Scancode::NumpadClear },
    { SDL_SCANCODE_KP_PERIOD, Scancode::NumpadDecimal },
    { SDL_SCANCODE_KP_DIVIDE, Scancode::NumpadDivide },
    { SDL_SCANCODE_KP_ENTER, Scancode::NumpadEnter },
    { SDL_SCANCODE_KP_EQUALS, Scancode::NumpadEqual },
    { SDL_SCANCODE_KP_MINUS, Scancode::NumpadMinus },
    { SDL_SCANCODE_KP_MULTIPLY, Scancode::NumpadMultiply },
    { SDL_SCANCODE_KP_PLUS, Scancode::NumpadAdd },
    { SDL_SCANCODE_KP_0, Scancode::Numpad0 },
    { SDL_SCANCODE_KP_1, Scancode::Numpad1 },
    { SDL_SCANCODE_KP_2, Scancode::Numpad2 },
    { SDL_SCANCODE_KP_3, Scancode::Numpad3 },
    { SDL_SCANCODE_KP_4, Scancode::Numpad4 },
    { SDL_SCANCODE_KP_5, Scancode::Numpad5 },
    { SDL_SCANCODE_KP_6, Scancode::Numpad6 },
    { SDL_SCANCODE_KP_7, Scancode::Numpad7 },
    { SDL_SCANCODE_KP_8, Scancode::Numpad8 },
    { SDL_SCANCODE_KP_9, Scancode::Numpad9 },
    { SDL_SCANCODE_INSERT, Scancode::Insert },
    { SDL_SCANCODE_PRINTSCREEN, Scancode::PrintScreen },
    { SDL_SCANCODE_SCROLLLOCK, Scancode::ScrollLock },
    { SDL_SCANCODE_PAUSE, Scancode::Pause },
    { SDL_SCANCODE_NUMLOCKCLEAR, Scancode::NumLock },
    { SDL_SCANCODE_APPLICATION, Scancode::ContextMenu },
    { SDL_SCANCODE_RGUI, Scancode::MetaRight },
    { SDL_SCANCODE_NONUSBACKSLASH, Scancode::IntlBackslash }
};

static Scancode _KeyConvert(SDL_Scancode scancode) {
    PROFILE

    auto const it = _keyMap.find(scancode);

    return (it != _keyMap.end()) ? it->second : Scancode::Unknown;
}

static KeyModifiers _ModifiersConvert(SDL_Keymod mod) {
    PROFILE

    return {
        .control = ((mod & SDL_KMOD_CTRL) != 0),
        .shift = ((mod & SDL_KMOD_SHIFT) != 0),
        .meta = ((mod & SDL_KMOD_GUI) != 0),
        .alt = ((mod & SDL_KMOD_ALT) != 0)
    };
}

static bool _IsControlCodepoint(std::uint32_t codepoint) {
    PROFILE

    return (codepoint < 0x20u) || ((codepoint >= 0x7Fu) && (codepoint < 0xA0u));
}

/* What the key means on the current layout: character keys take the
   character the layout gives them without modifiers (lowercased, so Shift
   does not change it); every other key, and a character key that yields no
   usable character (a dead key, a control character), keeps its name. */
static std::string _GetKeycode(SDL_Scancode sdlScancode, Scancode scancode) {
    PROFILE

    if (__IsCharacterScancode(scancode) == false) {
        return GetDefaultKeycode(scancode);
    }

    auto const key = SDL_GetKeyFromScancode(sdlScancode, SDL_KMOD_NONE, false);

    if ((key == SDLK_UNKNOWN) || ((key & SDLK_SCANCODE_MASK) != 0) || _IsControlCodepoint(key)) {
        return GetDefaultKeycode(scancode);
    }

    auto keycode = std::string();
    StringFromCodepoints({ CodepointToLower(key) }, keycode);

    return keycode;
}

std::string __GetLayoutKeycode(Scancode scancode) {
    PROFILE

    __InitVideo();

    for (auto const& [sdlScancode, mapped] : _keyMap) {
        if (mapped == scancode) {
            return _GetKeycode(sdlScancode, scancode);
        }
    }

    return GetDefaultKeycode(scancode);
}

static std::string _SanitizeInput(char const* text) {
    PROFILE

    if (text == nullptr) {
        return "";
    }

    static thread_local auto _codepoints = std::vector<std::uint32_t>();
    static thread_local auto _kept = std::vector<std::uint32_t>();

    StringToCodepoints(text, _codepoints);

    _kept.clear();

    for (auto codepoint : _codepoints) {
        if (_IsControlCodepoint(codepoint) == false) {
            _kept.push_back(codepoint);
        }
    }

    auto string = std::string();

    StringFromCodepoints(_kept, string);

    return string;
}

static std::optional<Mouse> _MouseConvert(std::uint8_t button) {
    PROFILE

    switch (button) {
        case SDL_BUTTON_LEFT:   return Mouse::LeftButton;
        case SDL_BUTTON_RIGHT:  return Mouse::RightButton;
        case SDL_BUTTON_MIDDLE: return Mouse::MiddleButton;
        default:                return std::nullopt; /* buttons past the third (back, forward) are not published */
    }
}

static SDL_SystemCursor _CursorConvert(Cursor cursor) {
    PROFILE

    switch (cursor) {
        case Cursor::Default:      return SDL_SYSTEM_CURSOR_DEFAULT;
        case Cursor::ContextMenu:  return SDL_SYSTEM_CURSOR_DEFAULT;     // NOTE: Not supported in SDL3
        case Cursor::Help:         return SDL_SYSTEM_CURSOR_DEFAULT;     // NOTE: Not supported in SDL3
        case Cursor::Pointer:      return SDL_SYSTEM_CURSOR_POINTER;
        case Cursor::Progress:     return SDL_SYSTEM_CURSOR_PROGRESS;
        case Cursor::Wait:         return SDL_SYSTEM_CURSOR_WAIT;
        case Cursor::Cell:         return SDL_SYSTEM_CURSOR_CROSSHAIR;   // NOTE: Not supported in SDL3
        case Cursor::Crosshair:    return SDL_SYSTEM_CURSOR_CROSSHAIR;
        case Cursor::Text:         return SDL_SYSTEM_CURSOR_TEXT;
        case Cursor::VerticalText: return SDL_SYSTEM_CURSOR_TEXT;        // NOTE: Not supported in SDL3
        case Cursor::Alias:        return SDL_SYSTEM_CURSOR_DEFAULT;     // NOTE: Not supported in SDL3
        case Cursor::Copy:         return SDL_SYSTEM_CURSOR_DEFAULT;     // NOTE: Not supported in SDL3
        case Cursor::Move:         return SDL_SYSTEM_CURSOR_MOVE;
        case Cursor::NoDrop:       return SDL_SYSTEM_CURSOR_NOT_ALLOWED; // NOTE: Not supported in SDL3
        case Cursor::NotAllowed:   return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
        case Cursor::Grab:         return SDL_SYSTEM_CURSOR_POINTER;     // NOTE: Not supported in SDL3
        case Cursor::Grabbing:     return SDL_SYSTEM_CURSOR_POINTER;     // NOTE: Not supported in SDL3
        case Cursor::EResize:      return SDL_SYSTEM_CURSOR_E_RESIZE;
        case Cursor::NResize:      return SDL_SYSTEM_CURSOR_N_RESIZE;
        case Cursor::NEResize:     return SDL_SYSTEM_CURSOR_NE_RESIZE;
        case Cursor::NWResize:     return SDL_SYSTEM_CURSOR_NW_RESIZE;
        case Cursor::SResize:      return SDL_SYSTEM_CURSOR_S_RESIZE;
        case Cursor::SEResize:     return SDL_SYSTEM_CURSOR_SE_RESIZE;
        case Cursor::SWResize:     return SDL_SYSTEM_CURSOR_SW_RESIZE;
        case Cursor::WResize:      return SDL_SYSTEM_CURSOR_W_RESIZE;
        case Cursor::EWResize:     return SDL_SYSTEM_CURSOR_EW_RESIZE;
        case Cursor::NSResize:     return SDL_SYSTEM_CURSOR_NS_RESIZE;
        case Cursor::NESWResize:   return SDL_SYSTEM_CURSOR_NESW_RESIZE;
        case Cursor::ColResize:    return SDL_SYSTEM_CURSOR_EW_RESIZE;   // NOTE: Not supported in SDL3
        case Cursor::RowResize:    return SDL_SYSTEM_CURSOR_NS_RESIZE;   // NOTE: Not supported in SDL3
        case Cursor::AllScroll:    return SDL_SYSTEM_CURSOR_MOVE;        // NOTE: Not supported in SDL3
        case Cursor::ZoomIn:       return SDL_SYSTEM_CURSOR_DEFAULT;     // NOTE: Not supported in SDL3
        case Cursor::ZoomOut:      return SDL_SYSTEM_CURSOR_DEFAULT;     // NOTE: Not supported in SDL3
        case Cursor::None:         return SDL_SYSTEM_CURSOR_DEFAULT;     /* hidden instead, see __setCursor */
    }

    return SDL_SYSTEM_CURSOR_DEFAULT;
}

/* System cursors are created once and kept for the life of the process. */
static SDL_Cursor* _GetCursor(Cursor cursor) {
    PROFILE

    static auto _cursors = std::map<SDL_SystemCursor, SDL_Cursor*>();

    auto const systemCursor = _CursorConvert(cursor);
    auto it = _cursors.find(systemCursor);

    if (it == _cursors.end()) {
        it = _cursors.emplace(systemCursor, SDL_CreateSystemCursor(systemCursor)).first;
    }

    return it->second;
}

/* Input goes to the innermost modal window only, as under [NSApp runModalForWindow:]. */
static bool _IsInputEvent(std::uint32_t type) {
    PROFILE

    switch (type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_INPUT:
        case SDL_EVENT_TEXT_EDITING:
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
        case SDL_EVENT_PINCH_UPDATE:
        case SDL_EVENT_DROP_BEGIN:
        case SDL_EVENT_DROP_FILE:
        case SDL_EVENT_DROP_COMPLETE:
            return true;
        default:
            return false;
    }
}

/* Makes a window operation take effect before the call returns, so the getters
   report it. Only a shown window is synced: a hidden one has nothing on screen
   to wait for, and on Wayland the wait would run into SDL's timeout. */
static void _SyncWindow(SDL_Window* window) {
    PROFILE

    if ((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0) {
        SDL_SyncWindow(window);
    }
}

static void _HandleEvent(SDL_Event const& event) {
    PROFILE

    if (event.type == SDL_EVENT_QUIT) {
        App::_Close();
        return;
    }

    auto const window = SDL_GetWindowFromEvent(&event);

    if (window == nullptr) {
        return;
    }

    auto const id = SDL_GetWindowID(window);

    if (_IsInputEvent(event.type) && (_modalWindows.empty() == false) && (_modalWindows.back() != id)) {
        return;
    }

    auto const it = _handlers.find(id);

    if (it != _handlers.end()) {
        /* a copy: the handler may destroy its own window */
        auto const onEvent = it->second.onEvent;
        onEvent(event);
    }
}

static void _PaintWindows() {
    PROFILE

    auto ids = std::vector<SDL_WindowID>();

    for (auto const& [ id, handlers ] : _handlers) {
        ids.push_back(id);
    }

    for (auto const id : ids) {
        auto const it = _handlers.find(id);

        if (it == _handlers.end()) {
            continue; /* destroyed while an earlier window painted */
        }

        auto const window = SDL_GetWindowFromID(id);
        auto const flags = SDL_GetWindowFlags(window);

        /* nothing would be presented, and a vsync present can wait for a
           frame that never comes */
        if ((flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED)) != 0) {
            continue;
        }

        auto const onPaint = it->second.onPaint;
        onPaint();
    }
}

static std::uint64_t _GetFramePeriod() {
    PROFILE

    auto const mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    auto const refreshRate = ((mode != nullptr) && (mode->refresh_rate > 0.0f)) ? mode->refresh_rate : _DEFAULT_REFRESH_RATE;

    return static_cast<std::uint64_t>(1000000000.0 / static_cast<double>(refreshRate));
}

void __InitVideo() {
    PROFILE

    static auto _initialized = false;

    if (_initialized) {
        return;
    }

    /* A window never closes by itself, so the last one asking to close must not quit the app. */
    SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0");
    /* The document draws the composition itself (see Window::setInputArea). */
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) == false) {
        throw std::runtime_error(SDL_GetError());
    }

    _initialized = true;
}

/* Moves and wheel turns arrive at the mouse's own rate, often far above the
   frame rate, and every one of them can cost a layout. macOS coalesces them to
   the display, and so does the run loop: they are held until the frame is due
   or another event comes, consecutive moves of a window collapse into the last
   one and consecutive turns into their sum. */
static bool _IsCoalescedEvent(SDL_Event const& event) {
    PROFILE

    return (event.type == SDL_EVENT_MOUSE_MOTION)
        || (event.type == SDL_EVENT_MOUSE_WHEEL);
}

static void _CoalesceEvent(std::vector<SDL_Event>& held, SDL_Event const& event) {
    PROFILE

    if (held.empty() == false) {
        auto& last = held.back();

        if (
            (event.type == SDL_EVENT_MOUSE_MOTION) &&
            (last.type == SDL_EVENT_MOUSE_MOTION) &&
            (last.motion.windowID == event.motion.windowID) &&
            (last.motion.state == event.motion.state)
        ) {
            last = event;
            return;
        }

        if (
            (event.type == SDL_EVENT_MOUSE_WHEEL) &&
            (last.type == SDL_EVENT_MOUSE_WHEEL) &&
            (last.wheel.windowID == event.wheel.windowID)
        ) {
            last.wheel.x += event.wheel.x;
            last.wheel.y += event.wheel.y;
            last.wheel.mouse_x = event.wheel.mouse_x;
            last.wheel.mouse_y = event.wheel.mouse_y;
            return;
        }
    }

    held.push_back(event);
}

/* Held events carry no strings, so they outlive SDL's next poll, which frees
   the strings of the events before it: every other event is handled at once. */
static void _HandleHeldEvents(std::vector<SDL_Event>& held) {
    PROFILE

    /* taken out first: a handler may run a nested loop (a modal window), which holds its own */
    auto const events = std::move(held);
    held.clear();

    for (auto const& event : events) {
        _HandleEvent(event);
    }
}

void __Tick(bool update) {
    PROFILE

    __InitVideo();

    static thread_local auto _held = std::vector<SDL_Event>();

    auto event = SDL_Event{};

    /* wait for the frame, or for the first event that cannot be held */
    for (;;) {
        auto const now = SDL_GetTicksNS();
        auto const wait = (_nextFrame > now) ? (_nextFrame - now) : 0;

        if (SDL_WaitEventTimeout(&event, static_cast<Sint32>((wait + 999999) / 1000000)) == false) {
            break;
        }

        if (_IsCoalescedEvent(event)) {
            _CoalesceEvent(_held, event);
            continue;
        }

        _HandleHeldEvents(_held);
        _HandleEvent(event);
        break;
    }

    while (SDL_PollEvent(&event)) {
        if (_IsCoalescedEvent(event)) {
            _CoalesceEvent(_held, event);
        } else {
            _HandleHeldEvents(_held);
            _HandleEvent(event);
        }
    }

    _HandleHeldEvents(_held);

    if (update) {
        App::_Update();
    }

    if (SDL_GetTicksNS() >= _nextFrame) {
        _nextFrame = (SDL_GetTicksNS() + _GetFramePeriod());
        _PaintWindows();
    }
}

void Window::__done() {
    PROFILE

    if (_impl->cursorHidden) {
        SDL_ShowCursor(); /* balance the app-global hide from __setCursor(Cursor::None) */
    }

    auto const id = SDL_GetWindowID(_impl->window);

    _handlers.erase(id);
    std::erase(_modalWindows, id);

    SDL_ReleaseWindowFromGPUDevice(static_cast<SDL_GPUDevice*>(__GetDefaultGPUDevice()), _impl->window);
    SDL_DestroyWindow(_impl->window);

    delete _impl;
}

void Window::__init() {
    PROFILE

    __InitVideo();

    _impl = new Window::_Window{
        .window = nullptr,
        .size = { 0.0f, 0.0f },
        .position = { 0.0f, 0.0f },
        .scale = 1.0f,
        .visible = false,
        .closable = false,
        .minimizable = false,
        .maximizable = true,
        .maximized = false,
        .minimized = false,
        .hover = false,
        .cursorHidden = false,
        .composing = false,
        .modal = false,
        .hiddenSize = std::nullopt,
        .mousePosition = std::nullopt,
        .pressedButtons = {},
        .droppedFiles = {}
    };

    /* Created like the macOS window: hidden, titled only, not resizable. */
    _impl->window = SDL_CreateWindow("", 640, 480, (SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY));

    if (_impl->window == nullptr) {
        auto const error = std::string(SDL_GetError());
        delete _impl;
        throw std::runtime_error(error);
    }

    auto claimed = SDL_ClaimWindowForGPUDevice(static_cast<SDL_GPUDevice*>(__GetDefaultGPUDevice()), _impl->window);

    for (auto attempt = 1; (claimed == false) && (attempt < _CLAIM_ATTEMPTS); attempt++) {
        SDL_Delay(_CLAIM_RETRY_DELAY_MS);
        claimed = SDL_ClaimWindowForGPUDevice(static_cast<SDL_GPUDevice*>(__GetDefaultGPUDevice()), _impl->window);
    }

    if (claimed == false) {
        auto const error = std::string(SDL_GetError());
        SDL_DestroyWindow(_impl->window);
        delete _impl;
        throw std::runtime_error(error);
    }

    _impl->scale = SDL_GetWindowPixelDensity(_impl->window);

    auto const modifiers = []() { return _ModifiersConvert(SDL_GetModState()); };

    auto const releaseButtons = [this](Vec2 const& position) {
        auto const pressed = _impl->pressedButtons;
        _impl->pressedButtons.clear();
        for (auto const mouse : pressed) {
            _mouseUp(mouse, position, _ModifiersConvert(SDL_GetModState()));
        }
    };

    auto const dispatchMouseMove = [this, modifiers](Vec2 const& position) {
        _impl->mousePosition = position;
        _mouseMove(position, modifiers());
    };

    auto const refreshScale = [this]() {
        auto const scale = SDL_GetWindowPixelDensity(_impl->window);
        if (scale != _impl->scale) {
            _impl->scale = scale;
            _dpiChange();
            _resize();
        }
    };

    _handlers[SDL_GetWindowID(_impl->window)] = _WindowHandlers{
        .onEvent = [this, modifiers, releaseButtons, dispatchMouseMove, refreshScale](SDL_Event const& event) {
            switch (event.type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    _close();
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED: {
                    _resize();
                    break;
                }
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
                    refreshScale();
                    break;
                }
                case SDL_EVENT_WINDOW_MAXIMIZED: {
                    if (_impl->maximized == false) {
                        _impl->maximized = true;
                        _maximize();
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_MINIMIZED: {
                    if (_impl->minimized == false) {
                        _impl->minimized = true;
                        _minimize();
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_RESTORED: {
                    /* one event for both: what the window leaves is the state it was in */
                    if (_impl->minimized) {
                        _impl->minimized = false;
                        _deminimize();
                    } else if (_impl->maximized) {
                        _impl->maximized = false;
                        _demaximize();
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_MOUSE_ENTER: {
                    auto position = Vec2{};
                    SDL_GetMouseState(&position.x, &position.y);
                    _impl->hover = true;
                    __setCursor(getCursor()); /* SDL's cursor is global: put this window's back */
                    _mouseEnter(position, modifiers());
                    break;
                }
                case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                    auto position = Vec2{};
                    SDL_GetMouseState(&position.x, &position.y);
                    _impl->hover = false;
                    _mouseExit(position, modifiers());
                    break;
                }
                case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                    _focus();

                    /* report the cursor position on activation to refresh hover without a move */
                    if (SDL_GetMouseFocus() == _impl->window) {
                        auto position = Vec2{};
                        SDL_GetMouseState(&position.x, &position.y);
                        dispatchMouseMove(position);
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_FOCUS_LOST: {
                    _blur();
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION: {
                    auto const position = Vec2{ event.motion.x, event.motion.y };

                    /* a move with no button held ends a press whose release never
                       arrived (the window manager took it): keep down/up paired */
                    if ((event.motion.state == 0) && (_impl->pressedButtons.empty() == false)) {
                        releaseButtons(position);
                    }

                    dispatchMouseMove(position);
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                    auto const mouse = _MouseConvert(event.button.button);
                    if (mouse.has_value() == false) {
                        break;
                    }

                    auto const position = Vec2{ event.button.x, event.button.y };

                    if (_impl->mousePosition != position) {
                        dispatchMouseMove(position);
                    }

                    if (std::ranges::find(_impl->pressedButtons, *mouse) == _impl->pressedButtons.end()) {
                        _impl->pressedButtons.push_back(*mouse);
                    }

                    _mouseDown(*mouse, position, modifiers(), static_cast<int>(event.button.clicks));
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    auto const mouse = _MouseConvert(event.button.button);
                    if (mouse.has_value() == false) {
                        break;
                    }

                    std::erase(_impl->pressedButtons, *mouse);

                    _mouseUp(*mouse, { event.button.x, event.button.y }, modifiers());
                    break;
                }
                case SDL_EVENT_MOUSE_WHEEL: {
                    /* the horizontal sign follows macOS's scrolling delta, which is
                       positive to the left */
                    _mouseWheel({ (event.wheel.x * -_WHEEL_SCALE), (event.wheel.y * _WHEEL_SCALE) }, modifiers());
                    break;
                }
                case SDL_EVENT_PINCH_UPDATE: {
                    /* A pinch is reported as a wheel turn with Control held, as Chrome does it: the turn is
                       100 times the natural logarithm of the change in scale, so a listener that multiplies
                       its scale by exp(wheel.y / 100) follows the fingers. */
                    if (event.pinch.scale > 0.0f) {
                        auto pinchModifiers = modifiers();
                        pinchModifiers.control = true;

                        _mouseWheel({ 0.0f, (std::log(event.pinch.scale) * 100.0f) }, pinchModifiers);
                    }
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    /* composition in progress: the input method owns the keys until it commits */
                    if (_impl->composing) {
                        break;
                    }

                    auto const scancode = _KeyConvert(event.key.scancode);

                    _keyDown(scancode, _GetKeycode(event.key.scancode, scancode), _ModifiersConvert(event.key.mod), event.key.repeat);
                    break;
                }
                case SDL_EVENT_KEY_UP: {
                    auto const scancode = _KeyConvert(event.key.scancode);

                    _keyUp(scancode, _GetKeycode(event.key.scancode, scancode), _ModifiersConvert(event.key.mod));
                    break;
                }
                case SDL_EVENT_TEXT_INPUT: {
                    auto const input = _SanitizeInput(event.text.text); /* control characters never reach input */
                    if (input.empty() == false) {
                        _input(input);
                    }
                    break;
                }
                case SDL_EVENT_DROP_BEGIN: {
                    _impl->droppedFiles.clear();
                    break;
                }
                case SDL_EVENT_DROP_FILE: {
                    if (event.drop.data != nullptr) {
                        _impl->droppedFiles.push_back(std::string(event.drop.data));
                    }
                    break;
                }
                case SDL_EVENT_DROP_COMPLETE: {
                    /* one drop of several files arrives as one file event each; they are published together */
                    auto const paths = _impl->droppedFiles;
                    _impl->droppedFiles.clear();

                    if (paths.empty() == false) {
                        _dropFiles(paths, Vec2{ event.drop.x, event.drop.y });
                    }
                    break;
                }
                case SDL_EVENT_TEXT_EDITING: {
                    auto const text = std::string((event.edit.text != nullptr) ? event.edit.text : "");
                    _impl->composing = (text.empty() == false);
                    _composition(text, std::max(event.edit.start, 0), std::max(event.edit.length, 0));
                    break;
                }
                default: {
                    break;
                }
            }
        },
        .onPaint = [this]() {
            _paint();
        }
    };
}

Vec2 const& Window::__getSize() const {
    PROFILE

    if (_impl->hiddenSize.has_value()) {
        _impl->size = *_impl->hiddenSize;
        return _impl->size;
    }

    auto width = 0;
    auto height = 0;

    SDL_GetWindowSize(_impl->window, &width, &height);

    _impl->size = { static_cast<float>(width), static_cast<float>(height) };

    return _impl->size;
}

float Window::__getScale() const {
    PROFILE

    _impl->scale = SDL_GetWindowPixelDensity(_impl->window);

    return _impl->scale;
}

void* Window::__getHandle() const {
    PROFILE

    return _impl->window;
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

    return ((SDL_GetWindowFlags(_impl->window) & SDL_WINDOW_RESIZABLE) != 0);
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

    return ((SDL_GetWindowFlags(_impl->window) & SDL_WINDOW_ALWAYS_ON_TOP) != 0);
}

bool Window::__isMaximized() const {
    PROFILE

    return ((SDL_GetWindowFlags(_impl->window) & SDL_WINDOW_MAXIMIZED) != 0);
}

bool Window::__isMinimized() const {
    PROFILE

    return ((SDL_GetWindowFlags(_impl->window) & SDL_WINDOW_MINIMIZED) != 0);
}

bool Window::__isHover() const {
    PROFILE

    return _impl->hover;
}

bool Window::__isFocused() const {
    PROFILE

    return ((SDL_GetWindowFlags(_impl->window) & SDL_WINDOW_INPUT_FOCUS) != 0);
}

void Window::__center() const {
    PROFILE

    auto const display = SDL_GetDisplayForWindow(_impl->window);

    SDL_SetWindowPosition(_impl->window, SDL_WINDOWPOS_CENTERED_DISPLAY(display), SDL_WINDOWPOS_CENTERED_DISPLAY(display));
    _SyncWindow(_impl->window);
}

void Window::__paint() const {
    PROFILE

    /* The run loop paints every window once a frame anyway (see __Tick). */
}

void Window::__setTitle(std::string const& title) {
    PROFILE

    SDL_SetWindowTitle(_impl->window, title.c_str());
}

void Window::__setInputArea(std::optional<Vec4> const& caretRect) {
    PROFILE

    if (caretRect.has_value()) {
        auto const rect = SDL_Rect{
            static_cast<int>(std::floor(caretRect->x)),
            static_cast<int>(std::floor(caretRect->y)),
            static_cast<int>(std::ceil(caretRect->width)),
            static_cast<int>(std::ceil(caretRect->height))
        };

        SDL_SetTextInputArea(_impl->window, &rect, 0);

        if (SDL_TextInputActive(_impl->window) == false) {
            SDL_StartTextInput(_impl->window);
        }
    } else if (SDL_TextInputActive(_impl->window)) {
        SDL_ClearComposition(_impl->window); /* drop a composition in progress */
        SDL_StopTextInput(_impl->window);

        if (_impl->composing) {
            _impl->composing = false;
            _composition("", 0, 0);
        }
    }
}

void Window::__setCursor(Cursor cursor) {
    PROFILE

    /* SDL_HideCursor is app-global, as [NSCursor hide] is on macOS. */
    if (cursor == Cursor::None) {
        if (_impl->cursorHidden == false) {
            SDL_HideCursor();
            _impl->cursorHidden = true;
        }
    } else if (_impl->cursorHidden) {
        SDL_ShowCursor();
        _impl->cursorHidden = false;
    }

    /* SDL's cursor is global too: apply it only while the mouse is over this window */
    if (_impl->hover && (cursor != Cursor::None)) {
        SDL_SetCursor(_GetCursor(cursor));
    }
}

/* The display whose coordinate space position values are expressed in, and
   its bounds in global coordinates. */
static SDL_Rect _PositionDisplayBounds(SDL_Window* window) {
    PROFILE

    auto display = SDL_GetDisplayForWindow(window);

    if (display == 0) {
        display = SDL_GetPrimaryDisplay();
    }

    auto bounds = SDL_Rect{};

    SDL_GetDisplayBounds(display, &bounds);

    return bounds;
}

Vec2 const& Window::__getPosition() const {
    PROFILE

    auto const bounds = _PositionDisplayBounds(_impl->window);

    auto x = 0;
    auto y = 0;
    auto top = 0;
    auto left = 0;

    SDL_GetWindowPosition(_impl->window, &x, &y);
    SDL_GetWindowBordersSize(_impl->window, &top, &left, nullptr, nullptr);

    /* SDL places the content area; the position is the frame's corner */
    _impl->position = {
        static_cast<float>((x - left) - bounds.x),
        static_cast<float>((y - top) - bounds.y)
    };

    return _impl->position;
}

void Window::__setPosition(Vec2 const& position) {
    PROFILE

    auto const bounds = _PositionDisplayBounds(_impl->window);

    auto top = 0;
    auto left = 0;

    SDL_GetWindowBordersSize(_impl->window, &top, &left, nullptr, nullptr);

    SDL_SetWindowPosition(
        _impl->window,
        (bounds.x + static_cast<int>(std::lround(position.x)) + left),
        (bounds.y + static_cast<int>(std::lround(position.y)) + top)
    );
    _SyncWindow(_impl->window);
}

void Window::__setSize(Vec2 const& size) {
    PROFILE

    auto const width = static_cast<int>(std::lround(size.width));
    auto const height = static_cast<int>(std::lround(size.height));

    SDL_SetWindowSize(_impl->window, width, height);
    _SyncWindow(_impl->window);

    /* a hidden window is not synced, and some drivers apply its size only
       once it is shown: until then it has the size it was given */
    if ((SDL_GetWindowFlags(_impl->window) & SDL_WINDOW_HIDDEN) != 0) {
        _impl->hiddenSize = Vec2{ static_cast<float>(width), static_cast<float>(height) };
    }
}

void Window::__setVisible(bool visible) {
    PROFILE

    if (visible) {
        if (_impl->visible == false) {
            SDL_ShowWindow(_impl->window);
            SDL_RaiseWindow(_impl->window);
            _SyncWindow(_impl->window);
            _impl->hiddenSize = std::nullopt;
            _impl->visible = true;
            _show();
        }
    } else {
        if (_impl->visible) {
            SDL_HideWindow(_impl->window);
            _impl->visible = false;
            _hide();
        }
    }
}

void Window::__setClosable(bool closable) {
    PROFILE

    // NOTE: Not supported in SDL3
    _impl->closable = closable;
}

void Window::__setSizable(bool sizable) {
    PROFILE

    SDL_SetWindowResizable(_impl->window, sizable);
}

void Window::__setMaximizable(bool maximizable) {
    PROFILE

    // NOTE: Not supported in SDL3
    _impl->maximizable = maximizable;
}

void Window::__setMinimizable(bool minimizable) {
    PROFILE

    // NOTE: Not supported in SDL3
    _impl->minimizable = minimizable;
}

void Window::__setMaximized(bool maximized) {
    PROFILE

    if (maximized != isMaximized()) {
        if (maximized) {
            SDL_MaximizeWindow(_impl->window);
        } else {
            SDL_RestoreWindow(_impl->window);
        }
    }
}

void Window::__setMinimized(bool minimized) {
    PROFILE

    if (minimized != isMinimized()) {
        if (minimized) {
            SDL_MinimizeWindow(_impl->window);
        } else {
            SDL_RestoreWindow(_impl->window);
        }
    }
}

void Window::__setTopmost(bool topmost) {
    PROFILE

    SDL_SetWindowAlwaysOnTop(_impl->window, topmost);
}

void Window::__setFocus() {
    PROFILE

    if (_impl->visible == false) {
        SDL_ShowWindow(_impl->window);
        _SyncWindow(_impl->window);
        _impl->hiddenSize = std::nullopt;
        _impl->visible = true;
    }

    SDL_RaiseWindow(_impl->window);
}

void Window::__runModal() {
    PROFILE

    auto const id = SDL_GetWindowID(_impl->window);
    auto const parent = GetFocusedWindow();

    if ((parent != nullptr) && (parent != this)) {
        SDL_SetWindowParent(_impl->window, parent->_impl->window);
        SDL_SetWindowModal(_impl->window, true);
    }

    _impl->modal = true;
    _modalWindows.push_back(id);

    /* A nested run loop: windows keep painting, but App::_Update does not
       run, so timeouts, intervals and UpdateAppEvent wait, as on macOS. */
    while (_impl->modal) {
        __Tick(false);
    }

    std::erase(_modalWindows, id);

    if ((parent != nullptr) && (parent != this)) {
        SDL_SetWindowModal(_impl->window, false);
        SDL_SetWindowParent(_impl->window, nullptr);
    }
}

void Window::__stopModal() {
    PROFILE

    _impl->modal = false;
}

} /* namespace Rocket */
