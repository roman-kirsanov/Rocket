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

KeyDownWindowEvent::KeyDownWindowEvent(Window& window, Key const& key, KeyModifiers const& modifiers, std::string const& input)
    : WindowEvent(window)
    , _key(key)
    , _modifiers(modifiers)
    , _input(input)
{
    PROFILE
}

Key const& KeyDownWindowEvent::getKey() const {
    PROFILE

    return _key;
}

KeyModifiers const& KeyDownWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

std::string const& KeyDownWindowEvent::getInput() const {
    PROFILE

    return _input;
}

KeyUpWindowEvent::KeyUpWindowEvent(Window& window, Key const& key, KeyModifiers const& modifiers)
    : WindowEvent(window)
    , _key(key)
    , _modifiers(modifiers)
{
    PROFILE
}

Key const& KeyUpWindowEvent::getKey() const {
    PROFILE

    return _key;
}

KeyModifiers const& KeyUpWindowEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

} /* namespace Rocket */
