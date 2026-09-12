/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Node/NodeEvent.hpp>

namespace Rocket {

NodeEvent::NodeEvent(Node& node)
    : _node(node)
    , _stopPropagation(false)
    , _defaultPrevented(false)
{
    PROFILE
}

Node& NodeEvent::getNode() const {
    PROFILE

    return _node;
}

void NodeEvent::stopPropagation() const {
    PROFILE

    _stopPropagation = true;
}

void NodeEvent::preventDefault() const {
    PROFILE

    _defaultPrevented = true;
}

bool NodeEvent::isDefaultPrevented() const {
    PROFILE

    return _defaultPrevented;
}

MouseClickNodeEvent::MouseClickNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseClickNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseClickNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseMoveNodeEvent::MouseMoveNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseMoveNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseMoveNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseEnterNodeEvent::MouseEnterNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseEnterNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseEnterNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseExitNodeEvent::MouseExitNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseExitNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseExitNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseWheelNodeEvent::MouseWheelNodeEvent(Node& node, Vec2 const& wheel, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _wheel(wheel)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseWheelNodeEvent::getWheel() const {
    PROFILE

    return _wheel;
}

KeyModifiers const& MouseWheelNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseDownNodeEvent::MouseDownNodeEvent(Node& node, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers, int clickCount)
    : NodeEvent(node)
    , _mouse(mouse)
    , _position(position)
    , _modifiers(modifiers)
    , _clickCount(clickCount)
{
    PROFILE
}

int MouseDownNodeEvent::getClickCount() const {
    PROFILE

    return _clickCount;
}

Mouse const& MouseDownNodeEvent::getMouse() const {
    PROFILE

    return _mouse;
}

Vec2 const& MouseDownNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseDownNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseUpNodeEvent::MouseUpNodeEvent(Node& node, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _mouse(mouse)
    , _position(position)
    , _modifiers(modifiers)
{
    PROFILE
}

Mouse const& MouseUpNodeEvent::getMouse() const {
    PROFILE

    return _mouse;
}

Vec2 const& MouseUpNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

KeyModifiers const& MouseUpNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseBeginDragNodeEvent::MouseBeginDragNodeEvent(Node& node, Vec2 const& position, Vec2 const& translate, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _position(position)
    , _translate(translate)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseBeginDragNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

Vec2 const& MouseBeginDragNodeEvent::getTranslate() const {
    PROFILE

    return _translate;
}

KeyModifiers const& MouseBeginDragNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseEndDragNodeEvent::MouseEndDragNodeEvent(Node& node, Vec2 const& position, Vec2 const& translate, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _position(position)
    , _translate(translate)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseEndDragNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

Vec2 const& MouseEndDragNodeEvent::getTranslate() const {
    PROFILE

    return _translate;
}

KeyModifiers const& MouseEndDragNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

MouseDragNodeEvent::MouseDragNodeEvent(Node& node, Vec2 const& position, Vec2 const& translate, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _position(position)
    , _translate(translate)
    , _modifiers(modifiers)
{
    PROFILE
}

Vec2 const& MouseDragNodeEvent::getPosition() const {
    PROFILE

    return _position;
}

Vec2 const& MouseDragNodeEvent::getTranslate() const {
    PROFILE

    return _translate;
}

KeyModifiers const& MouseDragNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

KeyDownNodeEvent::KeyDownNodeEvent(Node& node, Key const& key, KeyModifiers const& modifiers, std::string const& input)
    : NodeEvent(node)
    , _key(key)
    , _modifiers(modifiers)
    , _input(input)
{
    PROFILE
}

Key KeyDownNodeEvent::getKey() const {
    PROFILE

    return _key;
}

KeyModifiers const& KeyDownNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

std::string const& KeyDownNodeEvent::getInput() const {
    PROFILE

    return _input;
}

KeyUpNodeEvent::KeyUpNodeEvent(Node& node, Key const& key, KeyModifiers const& modifiers)
    : NodeEvent(node)
    , _key(key)
    , _modifiers(modifiers)
{
    PROFILE
}

Key KeyUpNodeEvent::getKey() const {
    PROFILE

    return _key;
}

KeyModifiers const& KeyUpNodeEvent::getModifiers() const {
    PROFILE

    return _modifiers;
}

InputNodeEvent::InputNodeEvent(Node& node, std::string const& content)
    : NodeEvent(node)
    , _content(content)
{
    PROFILE
}

std::string const& InputNodeEvent::getContent() const {
    PROFILE

    return _content;
}

} /* namespace Rocket */
