/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

namespace Rocket {

/** Mouse button identifier. */
enum class Mouse {
    /** The left (primary) mouse button. */
    LeftButton,

    /** The right (secondary) mouse button. */
    RightButton,

    /** The middle mouse button, usually the wheel. */
    MiddleButton
};

/**
 * Gamepad buttons, named after their position on the gamepad, not after the
 * label printed on them: South is A on an Xbox pad, Cross on a PlayStation
 * pad and B on a Nintendo pad.
 */
enum class GamepadButton {
    /** The bottom face button. */
    South,

    /** The right face button. */
    East,

    /** The left face button. */
    West,

    /** The top face button. */
    North,

    /** The Back (View, Select, Share) button. */
    Back,

    /** The Guide (Home) button. */
    Guide,

    /** The Start (Menu, Options) button. */
    Start,

    /** The left stick pressed in. */
    LeftStick,

    /** The right stick pressed in. */
    RightStick,

    /** The left shoulder button. */
    LeftShoulder,

    /** The right shoulder button. */
    RightShoulder,

    /** Up on the directional pad. */
    DPadUp,

    /** Down on the directional pad. */
    DPadDown,

    /** Left on the directional pad. */
    DPadLeft,

    /** Right on the directional pad. */
    DPadRight
};

/**
 * Gamepad axes. A stick axis goes from -1 to 1, negative to the left and up;
 * a trigger goes from 0 (released) to 1 (fully pressed).
 */
enum class GamepadAxis {
    /** The left stick, horizontally. */
    LeftX,

    /** The left stick, vertically. */
    LeftY,

    /** The right stick, horizontally. */
    RightX,

    /** The right stick, vertically. */
    RightY,

    /** The left trigger. */
    LeftTrigger,

    /** The right trigger. */
    RightTrigger
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
    /** A key the platform layer could not map. */
    Unknown,

    /** The 0 key of the digit row. */
    Digit0,

    /** The 1 key of the digit row. */
    Digit1,

    /** The 2 key of the digit row. */
    Digit2,

    /** The 3 key of the digit row. */
    Digit3,

    /** The 4 key of the digit row. */
    Digit4,

    /** The 5 key of the digit row. */
    Digit5,

    /** The 6 key of the digit row. */
    Digit6,

    /** The 7 key of the digit row. */
    Digit7,

    /** The 8 key of the digit row. */
    Digit8,

    /** The 9 key of the digit row. */
    Digit9,

    /** The A key. */
    KeyA,

    /** The B key. */
    KeyB,

    /** The C key. */
    KeyC,

    /** The D key. */
    KeyD,

    /** The E key. */
    KeyE,

    /** The F key. */
    KeyF,

    /** The G key. */
    KeyG,

    /** The H key. */
    KeyH,

    /** The I key. */
    KeyI,

    /** The J key. */
    KeyJ,

    /** The K key. */
    KeyK,

    /** The L key. */
    KeyL,

    /** The M key. */
    KeyM,

    /** The N key. */
    KeyN,

    /** The O key. */
    KeyO,

    /** The P key. */
    KeyP,

    /** The Q key. */
    KeyQ,

    /** The R key. */
    KeyR,

    /** The S key. */
    KeyS,

    /** The T key. */
    KeyT,

    /** The U key. */
    KeyU,

    /** The V key. */
    KeyV,

    /** The W key. */
    KeyW,

    /** The X key. */
    KeyX,

    /** The Y key. */
    KeyY,

    /** The Z key. */
    KeyZ,

    /** The F1 key. */
    F1,

    /** The F2 key. */
    F2,

    /** The F3 key. */
    F3,

    /** The F4 key. */
    F4,

    /** The F5 key. */
    F5,

    /** The F6 key. */
    F6,

    /** The F7 key. */
    F7,

    /** The F8 key. */
    F8,

    /** The F9 key. */
    F9,

    /** The F10 key. */
    F10,

    /** The F11 key. */
    F11,

    /** The F12 key. */
    F12,

    /** The F13 key. */
    F13,

    /** The F14 key. */
    F14,

    /** The F15 key. */
    F15,

    /** The F16 key. */
    F16,

    /** The F17 key. */
    F17,

    /** The F18 key. */
    F18,

    /** The F19 key. */
    F19,

    /** The F20 key. */
    F20,

    /** The left Alt key (Option on macOS). */
    Alt,

    /** The Down Arrow key. */
    ArrowDown,

    /** The Left Arrow key. */
    ArrowLeft,

    /** The Right Arrow key. */
    ArrowRight,

    /** The Up Arrow key. */
    ArrowUp,

    /** The Backslash key. */
    Backslash,

    /** The Backspace key (Delete on a Mac keyboard). */
    Backspace,

    /** The Left Bracket key. */
    BracketLeft,

    /** The Right Bracket key. */
    BracketRight,

    /** The Caps Lock key. */
    Capslock,

    /** The Comma key. */
    Comma,

    /** The left Control key. */
    Control,

    /** The End key. */
    End,

    /** The Equal key. */
    Equal,

    /** The Escape key. */
    Escape,

    /** The forward Delete key. */
    Delete,

    /** The Fn key. */
    Function,

    /** The Backquote key. */
    Backquote,

    /** The Help key. */
    Help,

    /** The Home key. */
    Home,

    /** The left Meta key (Command on macOS, the Windows key elsewhere). */
    Meta,

    /** The Minus key. */
    Minus,

    /** The Mute key. */
    Mute,

    /** The Page Down key. */
    PageDown,

    /** The Page Up key. */
    PageUp,

    /** The Period key. */
    Period,

    /** The Quote key. */
    Quote,

    /** The Enter key. */
    Enter,

    /** The right Alt key (Option on macOS). */
    AltRight,

    /** The right Control key. */
    ControlRight,

    /** The right Shift key. */
    ShiftRight,

    /** The Semicolon key. */
    Semicolon,

    /** The left Shift key. */
    Shift,

    /** The Slash key. */
    Slash,

    /** The Space bar. */
    Space,

    /** The Tab key. */
    Tab,

    /** The Volume Down key. */
    VolumeDown,

    /** The Volume Up key. */
    VolumeUp,

    /** The numpad Clear key. */
    NumpadClear,

    /** The numpad Decimal key. */
    NumpadDecimal,

    /** The numpad Divide key. */
    NumpadDivide,

    /** The numpad Enter key. */
    NumpadEnter,

    /** The numpad Equal key. */
    NumpadEqual,

    /** The numpad Minus key. */
    NumpadMinus,

    /** The numpad Multiply key. */
    NumpadMultiply,

    /** The numpad Add key. */
    NumpadAdd,

    /** The numpad 0 key. */
    Numpad0,

    /** The numpad 1 key. */
    Numpad1,

    /** The numpad 2 key. */
    Numpad2,

    /** The numpad 3 key. */
    Numpad3,

    /** The numpad 4 key. */
    Numpad4,

    /** The numpad 5 key. */
    Numpad5,

    /** The numpad 6 key. */
    Numpad6,

    /** The numpad 7 key. */
    Numpad7,

    /** The numpad 8 key. */
    Numpad8,

    /** The numpad 9 key. */
    Numpad9,

    /** The Insert key. */
    Insert,

    /** The Print Screen key. */
    PrintScreen,

    /** The Scroll Lock key. */
    ScrollLock,

    /** The Pause key. */
    Pause,

    /** The Num Lock key. */
    NumLock,

    /** The Context Menu key. */
    ContextMenu,

    /** The right Meta key (Command on macOS, the Windows key elsewhere). */
    MetaRight,

    /** The extra key between the left Shift and Z on ISO keyboards. */
    IntlBackslash
};

/** Modifier keys held during a keyboard or mouse event. */
struct KeyModifiers {
    /** Whether Control is held. */
    bool control;

    /** Whether Shift is held. */
    bool shift;

    /** Whether Meta is held: Command on macOS, the Windows key elsewhere. */
    bool meta;

    /** Whether Alt is held: Option on macOS. */
    bool alt;

    /**
     * Returns whether the same modifiers are held in both.
     *
     * @param modifiers The modifiers to compare with.
     */
    bool operator==(KeyModifiers const& modifiers) const;

    /**
     * Returns whether the modifiers held differ.
     *
     * @param modifiers The modifiers to compare with.
     */
    bool operator!=(KeyModifiers const& modifiers) const;
};

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

/** A key the platform layer could not identify. */
inline constexpr auto Unknown = std::string_view("Unidentified");

/** The A key. */
inline constexpr auto A = std::string_view("a");

/** The B key. */
inline constexpr auto B = std::string_view("b");

/** The C key. */
inline constexpr auto C = std::string_view("c");

/** The D key. */
inline constexpr auto D = std::string_view("d");

/** The E key. */
inline constexpr auto E = std::string_view("e");

/** The F key. */
inline constexpr auto F = std::string_view("f");

/** The G key. */
inline constexpr auto G = std::string_view("g");

/** The H key. */
inline constexpr auto H = std::string_view("h");

/** The I key. */
inline constexpr auto I = std::string_view("i");

/** The J key. */
inline constexpr auto J = std::string_view("j");

/** The K key. */
inline constexpr auto K = std::string_view("k");

/** The L key. */
inline constexpr auto L = std::string_view("l");

/** The M key. */
inline constexpr auto M = std::string_view("m");

/** The N key. */
inline constexpr auto N = std::string_view("n");

/** The O key. */
inline constexpr auto O = std::string_view("o");

/** The P key. */
inline constexpr auto P = std::string_view("p");

/** The Q key. */
inline constexpr auto Q = std::string_view("q");

/** The R key. */
inline constexpr auto R = std::string_view("r");

/** The S key. */
inline constexpr auto S = std::string_view("s");

/** The T key. */
inline constexpr auto T = std::string_view("t");

/** The U key. */
inline constexpr auto U = std::string_view("u");

/** The V key. */
inline constexpr auto V = std::string_view("v");

/** The W key. */
inline constexpr auto W = std::string_view("w");

/** The X key. */
inline constexpr auto X = std::string_view("x");

/** The Y key. */
inline constexpr auto Y = std::string_view("y");

/** The Z key. */
inline constexpr auto Z = std::string_view("z");

/** The 0 key. */
inline constexpr auto Digit0 = std::string_view("0");

/** The 1 key. */
inline constexpr auto Digit1 = std::string_view("1");

/** The 2 key. */
inline constexpr auto Digit2 = std::string_view("2");

/** The 3 key. */
inline constexpr auto Digit3 = std::string_view("3");

/** The 4 key. */
inline constexpr auto Digit4 = std::string_view("4");

/** The 5 key. */
inline constexpr auto Digit5 = std::string_view("5");

/** The 6 key. */
inline constexpr auto Digit6 = std::string_view("6");

/** The 7 key. */
inline constexpr auto Digit7 = std::string_view("7");

/** The 8 key. */
inline constexpr auto Digit8 = std::string_view("8");

/** The 9 key. */
inline constexpr auto Digit9 = std::string_view("9");

/** The Space bar. */
inline constexpr auto Space = std::string_view(" ");

/** The Minus key. */
inline constexpr auto Minus = std::string_view("-");

/** The Equal key. */
inline constexpr auto Equal = std::string_view("=");

/** The Left Bracket key. */
inline constexpr auto BracketLeft = std::string_view("[");

/** The Right Bracket key. */
inline constexpr auto BracketRight = std::string_view("]");

/** The Backslash key. */
inline constexpr auto Backslash = std::string_view("\\");

/** The Semicolon key. */
inline constexpr auto Semicolon = std::string_view(";");

/** The Quote key. */
inline constexpr auto Quote = std::string_view("'");

/** The Backquote key. */
inline constexpr auto Backquote = std::string_view("`");

/** The Comma key. */
inline constexpr auto Comma = std::string_view(",");

/** The Period key. */
inline constexpr auto Period = std::string_view(".");

/** The Slash key. */
inline constexpr auto Slash = std::string_view("/");

/** The Enter key. */
inline constexpr auto Enter = std::string_view("Enter");

/** The Tab key. */
inline constexpr auto Tab = std::string_view("Tab");

/** The Backspace key. */
inline constexpr auto Backspace = std::string_view("Backspace");

/** The forward Delete key. */
inline constexpr auto Delete = std::string_view("Delete");

/** The Escape key. */
inline constexpr auto Escape = std::string_view("Escape");

/** The Left Arrow key. */
inline constexpr auto ArrowLeft = std::string_view("ArrowLeft");

/** The Right Arrow key. */
inline constexpr auto ArrowRight = std::string_view("ArrowRight");

/** The Up Arrow key. */
inline constexpr auto ArrowUp = std::string_view("ArrowUp");

/** The Down Arrow key. */
inline constexpr auto ArrowDown = std::string_view("ArrowDown");

/** The Home key. */
inline constexpr auto Home = std::string_view("Home");

/** The End key. */
inline constexpr auto End = std::string_view("End");

/** The Page Up key. */
inline constexpr auto PageUp = std::string_view("PageUp");

/** The Page Down key. */
inline constexpr auto PageDown = std::string_view("PageDown");

/** The Help key. */
inline constexpr auto Help = std::string_view("Help");

/** The numpad Clear key. */
inline constexpr auto Clear = std::string_view("Clear");

/** The Insert key. */
inline constexpr auto Insert = std::string_view("Insert");

/** The Print Screen key. */
inline constexpr auto PrintScreen = std::string_view("PrintScreen");

/** The Scroll Lock key. */
inline constexpr auto ScrollLock = std::string_view("ScrollLock");

/** The Pause key. */
inline constexpr auto Pause = std::string_view("Pause");

/** The Context Menu key. */
inline constexpr auto ContextMenu = std::string_view("ContextMenu");

/** Either Shift key. */
inline constexpr auto Shift = std::string_view("Shift");

/** Either Control key. */
inline constexpr auto Control = std::string_view("Control");

/** Either Alt key (Option on macOS). */
inline constexpr auto Alt = std::string_view("Alt");

/** Either Meta key (Command on macOS, the Windows key elsewhere). */
inline constexpr auto Meta = std::string_view("Meta");

/** The Caps Lock key. */
inline constexpr auto CapsLock = std::string_view("CapsLock");

/** The Fn key. */
inline constexpr auto Fn = std::string_view("Fn");

/** The Num Lock key. */
inline constexpr auto NumLock = std::string_view("NumLock");

/** The Volume Up key. */
inline constexpr auto AudioVolumeUp = std::string_view("AudioVolumeUp");

/** The Volume Down key. */
inline constexpr auto AudioVolumeDown = std::string_view("AudioVolumeDown");

/** The Mute key. */
inline constexpr auto AudioVolumeMute = std::string_view("AudioVolumeMute");

/** The F1 key. */
inline constexpr auto F1 = std::string_view("F1");

/** The F2 key. */
inline constexpr auto F2 = std::string_view("F2");

/** The F3 key. */
inline constexpr auto F3 = std::string_view("F3");

/** The F4 key. */
inline constexpr auto F4 = std::string_view("F4");

/** The F5 key. */
inline constexpr auto F5 = std::string_view("F5");

/** The F6 key. */
inline constexpr auto F6 = std::string_view("F6");

/** The F7 key. */
inline constexpr auto F7 = std::string_view("F7");

/** The F8 key. */
inline constexpr auto F8 = std::string_view("F8");

/** The F9 key. */
inline constexpr auto F9 = std::string_view("F9");

/** The F10 key. */
inline constexpr auto F10 = std::string_view("F10");

/** The F11 key. */
inline constexpr auto F11 = std::string_view("F11");

/** The F12 key. */
inline constexpr auto F12 = std::string_view("F12");

/** The F13 key. */
inline constexpr auto F13 = std::string_view("F13");

/** The F14 key. */
inline constexpr auto F14 = std::string_view("F14");

/** The F15 key. */
inline constexpr auto F15 = std::string_view("F15");

/** The F16 key. */
inline constexpr auto F16 = std::string_view("F16");

/** The F17 key. */
inline constexpr auto F17 = std::string_view("F17");

/** The F18 key. */
inline constexpr auto F18 = std::string_view("F18");

/** The F19 key. */
inline constexpr auto F19 = std::string_view("F19");

/** The F20 key. */
inline constexpr auto F20 = std::string_view("F20");

} /* namespace Keycode */

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
 * Returns the keycode the scancode has on a US layout: the character for
 * character keys ("a", "1", "-"; numpad keys give their character, numpad
 * Enter gives "Enter") and the name for the others. Platform layers use it
 * for keys whose meaning does not depend on the layout.
 *
 * @param scancode The physical key.
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

/**
 * The modifiers editing shortcuts (Copy, Paste, Undo, Select All) are made
 * with on this platform, as Document matches them: Command (meta) on macOS,
 * Control elsewhere. Use it to send such a shortcut.
 */
KeyModifiers GetShortcutModifiers();

/**
 * Whether the modifiers hold the key editing shortcuts are made with on this
 * platform (see GetShortcutModifiers); other modifiers may be held with it.
 * Use it to recognize such a shortcut.
 *
 * @param modifiers The modifiers of a key event.
 */
bool HasShortcutModifier(KeyModifiers const& modifiers);

/**
 * The name to show the user for a key, as labelled on their keyboard: for a
 * key that types a character it is that character in uppercase, picked the
 * way GetShortcutKeycode picks it (AZERTY's KeyZ is "W", German's
 * Semicolon is "Ö", the digit row is always its digit, a key typing a
 * non-Latin letter is its Latin letter); for any other key, and for Space,
 * it is GetScancodeName. Use it in prompts and on rebinding screens; use
 * GetScancodeName where the name must not depend on the layout.
 *
 * @param scancode The physical key.
 * @param keycode  The key's keycode on the layout to name it for.
 */
std::string GetScancodeLabel(Scancode scancode, std::string const& keycode);

/**
 * GetScancodeLabel for the user's current keyboard layout. Call it on the
 * main thread.
 *
 * @param scancode The physical key.
 */
std::string GetScancodeLabel(Scancode scancode);

/**
 * Returns the ids of the connected gamepads, in the order they were
 * connected (see GamepadConnectAppEvent). Empty before App::Run starts and
 * after it returns.
 */
std::vector<std::uint32_t> GetGamepads();

/**
 * Returns the name of a connected gamepad as its driver reports it (e.g.
 * "Xbox Series X Controller", "PS5 Controller"), or an empty string for an
 * id that is not connected.
 *
 * @param gamepad The gamepad's id.
 */
std::string GetGamepadName(std::uint32_t gamepad);

} /* namespace Rocket */
