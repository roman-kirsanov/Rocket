#include <unordered_map>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/Input.hpp>

namespace Rocket {

bool KeyModifiers::operator==(KeyModifiers const& modifiers) const {
    PROFILE

    return (control == modifiers.control)
        && (shift == modifiers.shift)
        && (meta == modifiers.meta)
        && (alt == modifiers.alt);
}

bool KeyModifiers::operator!=(KeyModifiers const& modifiers) const {
    PROFILE

    return !operator==(modifiers);
}

std::string const& GetScancodeName(Scancode scancode) {
    PROFILE

    static auto _unknown = std::string("Unknown");
    static auto _namemap = std::unordered_map<Scancode, std::string>{
        { Scancode::Unknown, "Unknown" },
        { Scancode::Digit0, "0" },
        { Scancode::Digit1, "1" },
        { Scancode::Digit2, "2" },
        { Scancode::Digit3, "3" },
        { Scancode::Digit4, "4" },
        { Scancode::Digit5, "5" },
        { Scancode::Digit6, "6" },
        { Scancode::Digit7, "7" },
        { Scancode::Digit8, "8" },
        { Scancode::Digit9, "9" },
        { Scancode::KeyA, "A" },
        { Scancode::KeyB, "B" },
        { Scancode::KeyC, "C" },
        { Scancode::KeyD, "D" },
        { Scancode::KeyE, "E" },
        { Scancode::KeyF, "F" },
        { Scancode::KeyG, "G" },
        { Scancode::KeyH, "H" },
        { Scancode::KeyI, "I" },
        { Scancode::KeyJ, "J" },
        { Scancode::KeyK, "K" },
        { Scancode::KeyL, "L" },
        { Scancode::KeyM, "M" },
        { Scancode::KeyN, "N" },
        { Scancode::KeyO, "O" },
        { Scancode::KeyP, "P" },
        { Scancode::KeyQ, "Q" },
        { Scancode::KeyR, "R" },
        { Scancode::KeyS, "S" },
        { Scancode::KeyT, "T" },
        { Scancode::KeyU, "U" },
        { Scancode::KeyV, "V" },
        { Scancode::KeyW, "W" },
        { Scancode::KeyX, "X" },
        { Scancode::KeyY, "Y" },
        { Scancode::KeyZ, "Z" },
        { Scancode::F1, "F1" },
        { Scancode::F2, "F2" },
        { Scancode::F3, "F3" },
        { Scancode::F4, "F4" },
        { Scancode::F5, "F5" },
        { Scancode::F6, "F6" },
        { Scancode::F7, "F7" },
        { Scancode::F8, "F8" },
        { Scancode::F9, "F9" },
        { Scancode::F10, "F10" },
        { Scancode::F11, "F11" },
        { Scancode::F12, "F12" },
        { Scancode::F13, "F13" },
        { Scancode::F14, "F14" },
        { Scancode::F15, "F15" },
        { Scancode::F16, "F16" },
        { Scancode::F17, "F17" },
        { Scancode::F18, "F18" },
        { Scancode::F19, "F19" },
        { Scancode::F20, "F20" },
        { Scancode::Alt, "Alt" },
        { Scancode::ArrowDown, "ArrowDown" },
        { Scancode::ArrowLeft, "ArrowLeft" },
        { Scancode::ArrowRight, "ArrowRight" },
        { Scancode::ArrowUp, "ArrowUp" },
        { Scancode::Backslash, "\\" },
        { Scancode::Backspace, "Backspace" },
        { Scancode::BracketLeft, "[" },
        { Scancode::BracketRight, "]" },
        { Scancode::Capslock, "Capslock" },
        { Scancode::Comma, "," },
        { Scancode::Control, "Control" },
        { Scancode::End, "End" },
        { Scancode::Equal, "=" },
        { Scancode::Escape, "Escape" },
        { Scancode::Delete, "Delete" },
        { Scancode::Function, "Fn" },
        { Scancode::Backquote, "`" },
        { Scancode::Help, "Help" },
        { Scancode::Home, "Home" },
        { Scancode::Meta, "Meta" },
        { Scancode::Minus, "-" },
        { Scancode::Mute, "Mute" },
        { Scancode::PageDown, "PageDown" },
        { Scancode::PageUp, "PageUp" },
        { Scancode::Period, "." },
        { Scancode::Quote, "'" },
        { Scancode::Enter, "Enter" },
        { Scancode::AltRight, "AltRight" },
        { Scancode::ControlRight, "ControlRight" },
        { Scancode::ShiftRight, "ShiftRight" },
        { Scancode::Semicolon, ";" },
        { Scancode::Shift, "Shift" },
        { Scancode::Slash, "/" },
        { Scancode::Space, "Space" },
        { Scancode::Tab, "Tab" },
        { Scancode::VolumeDown, "VolumeDown" },
        { Scancode::VolumeUp, "VolumeUp" },
        { Scancode::NumpadClear, "NumpadClear" },
        { Scancode::NumpadDecimal, "NumpadDecimal" },
        { Scancode::NumpadDivide, "NumpadDivide" },
        { Scancode::NumpadEnter, "NumpadEnter" },
        { Scancode::NumpadEqual, "NumpadEqual" },
        { Scancode::NumpadMinus, "NumpadMinus" },
        { Scancode::NumpadMultiply, "NumpadMultiply" },
        { Scancode::NumpadAdd, "NumpadAdd" },
        { Scancode::Numpad0, "Numpad0" },
        { Scancode::Numpad1, "Numpad1" },
        { Scancode::Numpad2, "Numpad2" },
        { Scancode::Numpad3, "Numpad3" },
        { Scancode::Numpad4, "Numpad4" },
        { Scancode::Numpad5, "Numpad5" },
        { Scancode::Numpad6, "Numpad6" },
        { Scancode::Numpad7, "Numpad7" },
        { Scancode::Numpad8, "Numpad8" },
        { Scancode::Numpad9, "Numpad9" }
    };

    auto it = _namemap.find(scancode);
    if (it != _namemap.end()) {
        return it->second;
    } else {
        return _unknown;
    }
}

std::string GetDefaultKeycode(Scancode scancode) {
    PROFILE

    static auto const _keycodes = std::unordered_map<Scancode, std::string_view>{
        { Scancode::Digit0, Keycode::Digit0 },
        { Scancode::Digit1, Keycode::Digit1 },
        { Scancode::Digit2, Keycode::Digit2 },
        { Scancode::Digit3, Keycode::Digit3 },
        { Scancode::Digit4, Keycode::Digit4 },
        { Scancode::Digit5, Keycode::Digit5 },
        { Scancode::Digit6, Keycode::Digit6 },
        { Scancode::Digit7, Keycode::Digit7 },
        { Scancode::Digit8, Keycode::Digit8 },
        { Scancode::Digit9, Keycode::Digit9 },
        { Scancode::KeyA, Keycode::A },
        { Scancode::KeyB, Keycode::B },
        { Scancode::KeyC, Keycode::C },
        { Scancode::KeyD, Keycode::D },
        { Scancode::KeyE, Keycode::E },
        { Scancode::KeyF, Keycode::F },
        { Scancode::KeyG, Keycode::G },
        { Scancode::KeyH, Keycode::H },
        { Scancode::KeyI, Keycode::I },
        { Scancode::KeyJ, Keycode::J },
        { Scancode::KeyK, Keycode::K },
        { Scancode::KeyL, Keycode::L },
        { Scancode::KeyM, Keycode::M },
        { Scancode::KeyN, Keycode::N },
        { Scancode::KeyO, Keycode::O },
        { Scancode::KeyP, Keycode::P },
        { Scancode::KeyQ, Keycode::Q },
        { Scancode::KeyR, Keycode::R },
        { Scancode::KeyS, Keycode::S },
        { Scancode::KeyT, Keycode::T },
        { Scancode::KeyU, Keycode::U },
        { Scancode::KeyV, Keycode::V },
        { Scancode::KeyW, Keycode::W },
        { Scancode::KeyX, Keycode::X },
        { Scancode::KeyY, Keycode::Y },
        { Scancode::KeyZ, Keycode::Z },
        { Scancode::F1, Keycode::F1 },
        { Scancode::F2, Keycode::F2 },
        { Scancode::F3, Keycode::F3 },
        { Scancode::F4, Keycode::F4 },
        { Scancode::F5, Keycode::F5 },
        { Scancode::F6, Keycode::F6 },
        { Scancode::F7, Keycode::F7 },
        { Scancode::F8, Keycode::F8 },
        { Scancode::F9, Keycode::F9 },
        { Scancode::F10, Keycode::F10 },
        { Scancode::F11, Keycode::F11 },
        { Scancode::F12, Keycode::F12 },
        { Scancode::F13, Keycode::F13 },
        { Scancode::F14, Keycode::F14 },
        { Scancode::F15, Keycode::F15 },
        { Scancode::F16, Keycode::F16 },
        { Scancode::F17, Keycode::F17 },
        { Scancode::F18, Keycode::F18 },
        { Scancode::F19, Keycode::F19 },
        { Scancode::F20, Keycode::F20 },
        { Scancode::Alt, Keycode::Alt },
        { Scancode::AltRight, Keycode::Alt },
        { Scancode::ArrowDown, Keycode::ArrowDown },
        { Scancode::ArrowLeft, Keycode::ArrowLeft },
        { Scancode::ArrowRight, Keycode::ArrowRight },
        { Scancode::ArrowUp, Keycode::ArrowUp },
        { Scancode::Backslash, Keycode::Backslash },
        { Scancode::Backspace, Keycode::Backspace },
        { Scancode::BracketLeft, Keycode::BracketLeft },
        { Scancode::BracketRight, Keycode::BracketRight },
        { Scancode::Capslock, Keycode::CapsLock },
        { Scancode::Comma, Keycode::Comma },
        { Scancode::Control, Keycode::Control },
        { Scancode::ControlRight, Keycode::Control },
        { Scancode::End, Keycode::End },
        { Scancode::Equal, Keycode::Equal },
        { Scancode::Escape, Keycode::Escape },
        { Scancode::Delete, Keycode::Delete },
        { Scancode::Function, Keycode::Fn },
        { Scancode::Backquote, Keycode::Backquote },
        { Scancode::Help, Keycode::Help },
        { Scancode::Home, Keycode::Home },
        { Scancode::Meta, Keycode::Meta },
        { Scancode::Minus, Keycode::Minus },
        { Scancode::Mute, Keycode::AudioVolumeMute },
        { Scancode::PageDown, Keycode::PageDown },
        { Scancode::PageUp, Keycode::PageUp },
        { Scancode::Period, Keycode::Period },
        { Scancode::Quote, Keycode::Quote },
        { Scancode::Enter, Keycode::Enter },
        { Scancode::Semicolon, Keycode::Semicolon },
        { Scancode::Shift, Keycode::Shift },
        { Scancode::ShiftRight, Keycode::Shift },
        { Scancode::Slash, Keycode::Slash },
        { Scancode::Space, Keycode::Space },
        { Scancode::Tab, Keycode::Tab },
        { Scancode::VolumeDown, Keycode::AudioVolumeDown },
        { Scancode::VolumeUp, Keycode::AudioVolumeUp },
        { Scancode::NumpadClear, Keycode::Clear },
        { Scancode::NumpadDecimal, Keycode::Period },
        { Scancode::NumpadDivide, Keycode::Slash },
        { Scancode::NumpadEnter, Keycode::Enter },
        { Scancode::NumpadEqual, Keycode::Equal },
        { Scancode::NumpadMinus, Keycode::Minus },
        { Scancode::NumpadMultiply, std::string_view("*") },
        { Scancode::NumpadAdd, std::string_view("+") },
        { Scancode::Numpad0, Keycode::Digit0 },
        { Scancode::Numpad1, Keycode::Digit1 },
        { Scancode::Numpad2, Keycode::Digit2 },
        { Scancode::Numpad3, Keycode::Digit3 },
        { Scancode::Numpad4, Keycode::Digit4 },
        { Scancode::Numpad5, Keycode::Digit5 },
        { Scancode::Numpad6, Keycode::Digit6 },
        { Scancode::Numpad7, Keycode::Digit7 },
        { Scancode::Numpad8, Keycode::Digit8 },
        { Scancode::Numpad9, Keycode::Digit9 }
    };

    auto it = _keycodes.find(scancode);
    if (it != _keycodes.end()) {
        return std::string(it->second);
    } else {
        return std::string(Keycode::Unknown);
    }
}

std::string GetShortcutKeycode(Scancode scancode, std::string const& keycode) {
    PROFILE

    auto const digitRow = (scancode >= Scancode::Digit0) && (scancode <= Scancode::Digit9);
    auto const letterKey = (scancode >= Scancode::KeyA) && (scancode <= Scancode::KeyZ);
    auto const asciiCharacter = (keycode.size() == 1) && (static_cast<unsigned char>(keycode.front()) < 0x80u);

    if (digitRow || (letterKey && (asciiCharacter == false))) {
        return GetDefaultKeycode(scancode);
    }

    return keycode;
}

} /* namespace Rocket */
