/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/WindowEvent.hpp>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

WindowEvent::WindowEvent(Window& window)
    : _window(&window)
{
    PROFILE
}

Window& WindowEvent::getWindow() const {
    PROFILE

    return *_window;
}

MouseMoveWindowEvent::MouseMoveWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers)
    : WindowEvent(window)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseMoveWindowEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseMoveWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseEnterWindowEvent::MouseEnterWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers)
    : WindowEvent(window)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseEnterWindowEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseEnterWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseExitWindowEvent::MouseExitWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers)
    : WindowEvent(window)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseExitWindowEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseExitWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseWheelWindowEvent::MouseWheelWindowEvent(Window& window, Vec2 const& position, KeyModifiers const& modifiers)
    : WindowEvent(window)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseWheelWindowEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseWheelWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseDownWindowEvent::MouseDownWindowEvent(Window& window, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers, int clickCount)
    : WindowEvent(window)
    , _mouse(mouse)
    , _position(position)
    , _modifiers(modifiers)
    , _clickCount(clickCount)
{
    PROFILE
}

int MouseDownWindowEvent::getClickCount() const {
    PROFILE

    return _clickCount;
}

Mouse const& MouseDownWindowEvent::getMouse() const {
    PROFILE

    return _mouse;
}

Vec2 const& MouseDownWindowEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseDownWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseUpWindowEvent::MouseUpWindowEvent(Window& window, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers)
    : WindowEvent(window)
    , _mouse(mouse)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Mouse const& MouseUpWindowEvent::getMouse() const {
    PROFILE

    return _mouse;
}

Vec2 const& MouseUpWindowEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseUpWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

KeyDownWindowEvent::KeyDownWindowEvent(Window& window, Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers, bool repeat)
    : WindowEvent(window)
    , _scancode(scancode)
    , _keycode(keycode)
    , _modifiers(modifiers)
    , _repeat(repeat)
{
    PROFILE
}

Scancode KeyDownWindowEvent::getScancode() const {
    PROFILE

    return _scancode;
}

std::string const& KeyDownWindowEvent::getKeycode() const {
    PROFILE

    return _keycode;
}

KeyModifiers const& KeyDownWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

bool KeyDownWindowEvent::isRepeat() const {
    PROFILE

    return _repeat;
}

KeyUpWindowEvent::KeyUpWindowEvent(Window& window, Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers)
    : WindowEvent(window)
    , _scancode(scancode)
    , _keycode(keycode)
    , _modifiers(modifiers)
{
    PROFILE
}

Scancode KeyUpWindowEvent::getScancode() const {
    PROFILE

    return _scancode;
}

std::string const& KeyUpWindowEvent::getKeycode() const {
    PROFILE

    return _keycode;
}

KeyModifiers const& KeyUpWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

InputWindowEvent::InputWindowEvent(Window& window, std::string const& text)
    : WindowEvent(window)
    , _text(text)
{
    PROFILE
}

std::string const& InputWindowEvent::getText() const {
    PROFILE

    return _text;
}

CompositionWindowEvent::CompositionWindowEvent(Window& window, std::string const& text, std::int32_t cursor, std::int32_t selectionLength)
    : WindowEvent(window)
    , _text(text)
    , _cursor(cursor)
    , _selectionLength(selectionLength)
{
    PROFILE
}

std::string const& CompositionWindowEvent::getText() const {
    PROFILE

    return _text;
}

std::int32_t CompositionWindowEvent::getCursor() const {
    PROFILE

    return _cursor;
}

std::int32_t CompositionWindowEvent::getSelectionLength() const {
    PROFILE

    return _selectionLength;
}

} /* namespace Rocket */
