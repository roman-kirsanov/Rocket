/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>

namespace Rocket {

/** Mouse button identifier. */
enum class Mouse {
    LeftButton,
    RightButton,
    MiddleButton
};

/**
 * Physical keyboard keys, independent of layout and modifiers.
 *
 * Unknown represents any key the platform layer could not map.
 */
enum class Key {
    Unknown,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    KeyA,
    KeyB,
    KeyC,
    KeyD,
    KeyE,
    KeyF,
    KeyG,
    KeyH,
    KeyI,
    KeyJ,
    KeyK,
    KeyL,
    KeyM,
    KeyN,
    KeyO,
    KeyP,
    KeyQ,
    KeyR,
    KeyS,
    KeyT,
    KeyU,
    KeyV,
    KeyW,
    KeyX,
    KeyY,
    KeyZ,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    Alt,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    Backslash,
    Backspace,
    BracketLeft,
    BracketRight,
    Capslock,
    Comma,
    Control,
    End,
    Equal,
    Escape,
    Delete,
    Function,
    Backquote,
    Help,
    Home,
    Meta,
    Minus,
    Mute,
    PageDown,
    PageUp,
    Period,
    Quote,
    Enter,
    AltRight,
    ControlRight,
    ShiftRight,
    Semicolon,
    Shift,
    Slash,
    Space,
    Tab,
    VolumeDown,
    VolumeUp,
    NumpadClear,
    NumpadDecimal,
    NumpadDivide,
    NumpadEnter,
    NumpadEqual,
    NumpadMinus,
    NumpadMultiply,
    NumpadAdd,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9
};

/** Modifier keys held during a keyboard or mouse event. */
struct KeyModifiers {
    bool control;
    bool shift;
    bool meta;
    bool alt;
};

/**
 * Returns a human-readable name for a key (e.g. "A", "0", "Escape").
 *
 * Returns "Unknown" for Key::Unknown and for any unmapped value. The
 * returned reference is stable for the lifetime of the process.
 *
 * @param key The physical key to name.
 */
std::string const& GetKeyName(Key key);

} /* namespace Rocket */
