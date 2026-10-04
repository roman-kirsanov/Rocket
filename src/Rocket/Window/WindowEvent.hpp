/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <cstdint>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Base/Object.hpp>
#include <Rocket/Window/Input.hpp>

namespace Rocket {

class Window;

/**
 * Base class for all window events published on Window::onEvent.
 *
 * Every event carries the window it originated from. Events are dispatched
 * first to the window's protected virtual _onEvent, then published to
 * onEvent subscribers.
 */
class WindowEvent : public Object {
public:
    /**
     * An event originating from the given window.
     *
     * @param window The window the event originated from.
     */
    WindowEvent(Window& window);

    /** Returns the window this event originated from. */
    Window& getWindow() const;
private:
    Window* _window;
};

/** The window became visible (see Window::setVisible). */
class ShowWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window was hidden (see Window::setVisible). */
class HideWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The user activated the window's close control; the window is not closed automatically. */
class CloseWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window's size changed (query Window::getSize for the new size). Also published when the backing scale factor changes, even if the size did not. */
class ResizeWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window was maximized (enters full-screen mode on macOS). */
class MaximizeWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window was minimized to the dock/taskbar. */
class MinimizeWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window returned from the maximized state. */
class DemaximizeWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window returned from the minimized state. */
class DeminimizeWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window's backing scale factor changed (e.g. moved between displays). */
class DPIChangeWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The window's display is ready for the next frame (vsync-paced, tracks the screen the window is on); render in response. */
class PaintWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/** The mouse moved over the window; position is in window content coordinates. Published while moving and during left-button drags; not during right-button drags on macOS. */
class MouseMoveWindowEvent : public WindowEvent {
public:
    /**
     * An event at the given position with the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param position  The mouse position, in window content coordinates.
     * @param modifiers The modifier keys held at the time.
     */
    MouseMoveWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the mouse position in window content coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/**
 * The mouse entered the window's content area (Window::isHover becomes true).
 */
class MouseEnterWindowEvent : public WindowEvent {
public:
    /**
     * An event at the given position with the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param position  The mouse position, in window content coordinates.
     * @param modifiers The modifier keys held at the time.
     */
    MouseEnterWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the mouse position in window content coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/**
 * The mouse left the window's content area (Window::isHover becomes false).
 */
class MouseExitWindowEvent : public WindowEvent {
public:
    /**
     * An event at the given position with the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param position  The mouse position, in window content coordinates.
     * @param modifiers The modifier keys held at the time.
     */
    MouseExitWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the mouse position in window content coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/**
 * The mouse wheel or trackpad scrolled.
 *
 * Note: getPosition returns the scroll delta (x/y), not a cursor position.
 */
class MouseWheelWindowEvent : public WindowEvent {
public:
    /**
     * An event carrying the scroll delta and the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param position  The scroll delta (x/y).
     * @param modifiers The modifier keys held at the time.
     */
    MouseWheelWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the scroll delta for this event. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/**
 * A mouse button was pressed; position is in window content coordinates.
 *
 * Every press is preceded by a MouseMoveWindowEvent to the same position,
 * so hover state is current before the press is handled. Platform backends
 * must uphold this, synthesizing the move when the platform does not
 * deliver one.
 *
 * Note: the middle button is not currently published by the macOS backend
 * (the middle button is unhandled).
 */
class MouseDownWindowEvent : public WindowEvent {
public:
    /**
     * An event for the given button at the given position with the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param mouse     The mouse button that was pressed.
     * @param position  The mouse position, in window content coordinates.
     * @param modifiers  The modifier keys held at the time.
     * @param clickCount The number of consecutive clicks this press is part of (1 for a single click); default 1.
     */
    MouseDownWindowEvent(Window& window, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers, int clickCount = 1);

    /** Returns which mouse button was pressed. */
    Mouse const& getMouse() const;

    /** Returns the mouse position in window content coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;

    /** Returns the number of consecutive clicks this press is part of: 1 for a single click, 2 for a double click, and so on. */
    int getClickCount() const;
private:
    Mouse _mouse;
    Vec2 _position;
    KeyModifiers _modifiers;
    int _clickCount;
};

/**
 * A mouse button was released; position is in window content coordinates.
 *
 * Note: the middle button is not currently published by the macOS backend
 * (the middle button is unhandled).
 */
class MouseUpWindowEvent : public WindowEvent {
public:
    /**
     * An event for the given button at the given position with the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param mouse     The mouse button that was released.
     * @param position  The mouse position, in window content coordinates.
     * @param modifiers The modifier keys held at the time.
     */
    MouseUpWindowEvent(Window& window, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns which mouse button was released. */
    Mouse const& getMouse() const;

    /** Returns the mouse position in window content coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;
private:
    Mouse _mouse;
    Vec2 _position;
    KeyModifiers _modifiers;
};

/** A key was pressed while the window had focus. */
class KeyDownWindowEvent : public WindowEvent {
public:
    /**
     * An event for the given key and the held modifiers. Typed text does not
     * come with it: it arrives as an InputWindowEvent.
     *
     * @param window    The window the event originated from.
     * @param scancode  The physical key that was pressed.
     * @param keycode   What the key means on the current layout (see Keycode).
     * @param modifiers The modifier keys held at the time.
     * @param repeat    Whether the key is auto-repeating because it is held down.
     */
    KeyDownWindowEvent(Window& window, Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers, bool repeat = false);

    /** Returns the physical key that was pressed. */
    Scancode getScancode() const;

    /** Returns what the key means on the current layout (see Keycode); use it for shortcuts. */
    std::string const& getKeycode() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;

    /** Returns whether the key is auto-repeating because it is held down. */
    bool isRepeat() const;

private:
    Scancode _scancode;
    std::string _keycode;
    KeyModifiers _modifiers;
    bool _repeat;
};

/** A key was released while the window had focus. */
class KeyUpWindowEvent : public WindowEvent {
public:
    /**
     * An event for the given key and the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param scancode  The physical key that was released.
     * @param keycode   What the key means on the current layout (see Keycode).
     * @param modifiers The modifier keys held at the time.
     */
    KeyUpWindowEvent(Window& window, Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers);

    /** Returns the physical key that was released. */
    Scancode getScancode() const;

    /** Returns what the key means on the current layout (see Keycode). */
    std::string const& getKeycode() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;
private:
    Scancode _scancode;
    std::string _keycode;
    KeyModifiers _modifiers;
};

/**
 * Text the OS text system committed while the window had focus: typed
 * characters, the result of a dead key or input method, the emoji picker,
 * dictation. Arrives after the key event that produced it, if any (the
 * emoji picker and dictation produce none). Enter, Tab, Backspace and
 * shortcuts commit no text: they arrive as key events only.
 */
class InputWindowEvent : public WindowEvent {
public:
    /**
     * An event for committed text.
     *
     * @param window The window the event originated from.
     * @param text   The committed UTF-8 text.
     */
    InputWindowEvent(Window& window, std::string const& text);

    /** Returns the committed UTF-8 text. */
    std::string const& getText() const;
private:
    std::string _text;
};

/**
 * An input method's composition changed: the in-progress text it shows
 * before committing (e.g. "にほんご" before it becomes "日本語"), with the
 * caret and the selected segment inside it, in characters. Empty text means
 * the composition ended; when it ended by committing, an InputWindowEvent
 * with the result follows.
 */
class CompositionWindowEvent : public WindowEvent {
public:
    /**
     * An event for the current composition.
     *
     * @param window          The window the event originated from.
     * @param text            The in-progress UTF-8 text; empty when the composition ended.
     * @param cursor          The caret position inside the text, in characters.
     * @param selectionLength The length of the selected segment starting at the caret, in characters.
     */
    CompositionWindowEvent(Window& window, std::string const& text, std::int32_t cursor, std::int32_t selectionLength);

    /** Returns the in-progress UTF-8 text; empty when the composition ended. */
    std::string const& getText() const;

    /** Returns the caret position inside the text, in characters. */
    std::int32_t getCursor() const;

    /** Returns the length of the selected segment starting at the caret, in characters. */
    std::int32_t getSelectionLength() const;
private:
    std::string _text;
    std::int32_t _cursor;
    std::int32_t _selectionLength;
};

/**
 * The window gained keyboard focus.
 *
 * Note: not currently published by the macOS backend; query Window::isFocused instead.
 */
class FocusWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

/**
 * The window lost keyboard focus.
 *
 * Note: not currently published by the macOS backend; query Window::isFocused instead.
 */
class BlurWindowEvent : public WindowEvent {
public:
    using WindowEvent::WindowEvent;
};

} /* namespace Rocket */
