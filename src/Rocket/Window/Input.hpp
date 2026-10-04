/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <string_view>

namespace Rocket {

/** Mouse button identifier. */
enum class Mouse {
    LeftButton,
    RightButton,
    MiddleButton
};

/**
 * Physical keyboard keys, independent of layout and modifiers, named after
 * their position on a US keyboard (like the web's KeyboardEvent.code).
 *
 * Use a scancode for bindings that depend on where a key is (game controls);
 * use the keycode for shortcuts, which follow the key's label on the
 * user's layout. Unknown represents any key the platform layer could not map.
 */
enum class Scancode {
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

    bool operator==(KeyModifiers const&) const;
    bool operator!=(KeyModifiers const&) const;
};

/**
 * Returns a human-readable name for a scancode, as printed on a US keyboard
 * (e.g. "A", "0", "Escape").
 *
 * Returns "Unknown" for Scancode::Unknown and for any unmapped value. The
 * returned reference is stable for the lifetime of the process.
 *
 * @param scancode The physical key to name.
 */
std::string const& GetScancodeName(Scancode scancode);

/**
 * Keycodes: what a key means on the user's current layout (like the web's
 * KeyboardEvent.key). A keycode is a std::string: for a key that types a
 * character it is that character without modifiers, lowercase ("a", "ф",
 * "-", " " for the space bar); for any other key it is a name ("Enter",
 * "ArrowLeft", "F5"). A key that types more than one character holds them
 * all. The constants below name the common keycodes for comparisons, e.g.
 * `event.getKeycode() == Keycode::Enter`.
 */
namespace Keycode {

inline constexpr auto Unknown = std::string_view("Unidentified");

inline constexpr auto A = std::string_view("a");
inline constexpr auto B = std::string_view("b");
inline constexpr auto C = std::string_view("c");
inline constexpr auto D = std::string_view("d");
inline constexpr auto E = std::string_view("e");
inline constexpr auto F = std::string_view("f");
inline constexpr auto G = std::string_view("g");
inline constexpr auto H = std::string_view("h");
inline constexpr auto I = std::string_view("i");
inline constexpr auto J = std::string_view("j");
inline constexpr auto K = std::string_view("k");
inline constexpr auto L = std::string_view("l");
inline constexpr auto M = std::string_view("m");
inline constexpr auto N = std::string_view("n");
inline constexpr auto O = std::string_view("o");
inline constexpr auto P = std::string_view("p");
inline constexpr auto Q = std::string_view("q");
inline constexpr auto R = std::string_view("r");
inline constexpr auto S = std::string_view("s");
inline constexpr auto T = std::string_view("t");
inline constexpr auto U = std::string_view("u");
inline constexpr auto V = std::string_view("v");
inline constexpr auto W = std::string_view("w");
inline constexpr auto X = std::string_view("x");
inline constexpr auto Y = std::string_view("y");
inline constexpr auto Z = std::string_view("z");

inline constexpr auto Digit0 = std::string_view("0");
inline constexpr auto Digit1 = std::string_view("1");
inline constexpr auto Digit2 = std::string_view("2");
inline constexpr auto Digit3 = std::string_view("3");
inline constexpr auto Digit4 = std::string_view("4");
inline constexpr auto Digit5 = std::string_view("5");
inline constexpr auto Digit6 = std::string_view("6");
inline constexpr auto Digit7 = std::string_view("7");
inline constexpr auto Digit8 = std::string_view("8");
inline constexpr auto Digit9 = std::string_view("9");

inline constexpr auto Space = std::string_view(" ");
inline constexpr auto Minus = std::string_view("-");
inline constexpr auto Equal = std::string_view("=");
inline constexpr auto BracketLeft = std::string_view("[");
inline constexpr auto BracketRight = std::string_view("]");
inline constexpr auto Backslash = std::string_view("\\");
inline constexpr auto Semicolon = std::string_view(";");
inline constexpr auto Quote = std::string_view("'");
inline constexpr auto Backquote = std::string_view("`");
inline constexpr auto Comma = std::string_view(",");
inline constexpr auto Period = std::string_view(".");
inline constexpr auto Slash = std::string_view("/");

inline constexpr auto Enter = std::string_view("Enter");
inline constexpr auto Tab = std::string_view("Tab");
inline constexpr auto Backspace = std::string_view("Backspace");
inline constexpr auto Delete = std::string_view("Delete");
inline constexpr auto Escape = std::string_view("Escape");
inline constexpr auto ArrowLeft = std::string_view("ArrowLeft");
inline constexpr auto ArrowRight = std::string_view("ArrowRight");
inline constexpr auto ArrowUp = std::string_view("ArrowUp");
inline constexpr auto ArrowDown = std::string_view("ArrowDown");
inline constexpr auto Home = std::string_view("Home");
inline constexpr auto End = std::string_view("End");
inline constexpr auto PageUp = std::string_view("PageUp");
inline constexpr auto PageDown = std::string_view("PageDown");
inline constexpr auto Help = std::string_view("Help");
inline constexpr auto Clear = std::string_view("Clear");

inline constexpr auto Shift = std::string_view("Shift");
inline constexpr auto Control = std::string_view("Control");
inline constexpr auto Alt = std::string_view("Alt");
inline constexpr auto Meta = std::string_view("Meta");
inline constexpr auto CapsLock = std::string_view("CapsLock");
inline constexpr auto Fn = std::string_view("Fn");

inline constexpr auto AudioVolumeUp = std::string_view("AudioVolumeUp");
inline constexpr auto AudioVolumeDown = std::string_view("AudioVolumeDown");
inline constexpr auto AudioVolumeMute = std::string_view("AudioVolumeMute");

inline constexpr auto F1 = std::string_view("F1");
inline constexpr auto F2 = std::string_view("F2");
inline constexpr auto F3 = std::string_view("F3");
inline constexpr auto F4 = std::string_view("F4");
inline constexpr auto F5 = std::string_view("F5");
inline constexpr auto F6 = std::string_view("F6");
inline constexpr auto F7 = std::string_view("F7");
inline constexpr auto F8 = std::string_view("F8");
inline constexpr auto F9 = std::string_view("F9");
inline constexpr auto F10 = std::string_view("F10");
inline constexpr auto F11 = std::string_view("F11");
inline constexpr auto F12 = std::string_view("F12");
inline constexpr auto F13 = std::string_view("F13");
inline constexpr auto F14 = std::string_view("F14");
inline constexpr auto F15 = std::string_view("F15");
inline constexpr auto F16 = std::string_view("F16");
inline constexpr auto F17 = std::string_view("F17");
inline constexpr auto F18 = std::string_view("F18");
inline constexpr auto F19 = std::string_view("F19");
inline constexpr auto F20 = std::string_view("F20");

} /* namespace Keycode */

/**
 * Returns the keycode the scancode has on a US layout: the character for
 * character keys ("a", "1", "-"; numpad keys give their character, numpad
 * Enter gives "Enter") and the name for the others. Platform layers use it
 * for keys whose meaning does not depend on the layout.
 */
std::string GetDefaultKeycode(Scancode scancode);

/**
 * The keycode to match shortcuts against, following what the OS does for
 * its own shortcuts: the key's keycode, except that the digit row always
 * gives its digit (AZERTY types "&" there without Shift, yet Cmd+1 means the
 * key labelled 1) and a letter key that types a non-Latin character
 * (Cyrillic, Greek, Hebrew…) gives the Latin letter of its position, so
 * Cmd+Z / Ctrl+C work on every layout.
 *
 * @param scancode The physical key.
 * @param keycode  The key's keycode on the current layout.
 */
std::string GetShortcutKeycode(Scancode scancode, std::string const& keycode);

} /* namespace Rocket */
