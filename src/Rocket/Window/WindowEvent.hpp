/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
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
     * An event for the given key, the held modifiers, and the UTF-8 text the keypress produced.
     *
     * @param window    The window the event originated from.
     * @param key       The physical key that was pressed.
     * @param modifiers The modifier keys held at the time.
     * @param input     The UTF-8 text the keypress produced once the input context composed it (dead keys and input methods included); empty for non-text keys and for Command shortcuts.
     */
    KeyDownWindowEvent(Window& window, Key const& key, KeyModifiers const& modifiers, std::string const& input);

    /** Returns the physical key that was pressed. */
    Key const& getKey() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;

    /** Returns the UTF-8 text the keypress produced once the input context composed it; empty for non-text keys and for Command shortcuts. */
    std::string const& getInput() const;
private:
    Key _key;
    KeyModifiers _modifiers;
    std::string _input;
};

/** A key was released while the window had focus. */
class KeyUpWindowEvent : public WindowEvent {
public:
    /**
     * An event for the given key and the modifiers held at the time.
     *
     * @param window    The window the event originated from.
     * @param key       The physical key that was released.
     * @param modifiers The modifier keys held at the time.
     */
    KeyUpWindowEvent(Window& window, Key const& key, KeyModifiers const& modifiers);

    /** Returns the physical key that was released. */
    Key const& getKey() const;

    /** Returns the modifier keys held during the event. */
    KeyModifiers const& getModifiers() const;
private:
    Key _key;
    KeyModifiers _modifiers;
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
