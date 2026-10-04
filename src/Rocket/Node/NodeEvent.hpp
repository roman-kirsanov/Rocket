/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <Rocket/Base/Object.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Window/Input.hpp>

namespace Rocket {

class Node;

/**
 * Base class for all UI events.
 *
 * Carries a reference to the target Node and controls event propagation.
 * Dispatch walks the ancestor chain in two passes — capture (root to target)
 * then bubble (target back to root). Mouse positions are in document
 * coordinates (origin at the document's top-left, in document scale units).
 */
class NodeEvent : public Object {
public:
    /**
     * An event targeting the given node.
     *
     * @param node The node that is the target of the event.
     */
    NodeEvent(Node& node);

    /** Returns the Node that is the target of this event. */
    Node& getNode() const;

    /**
     * Stops further propagation along the dispatch path.
     *
     * Once called, no further node on the path receives the event in that
     * phase (descendants toward the target during capture, ancestors during
     * bubbling); the target node's own bubble-phase listeners still run even
     * when propagation was stopped during capture.
     */
    void stopPropagation() const;

    /**
     * Marks the event as default-prevented. For a KeyDownNodeEvent this
     * keeps the document from applying the key to the focused editable
     * text and from moving focus on Tab, so a listener can filter or
     * limit what is typed. For a MouseDownNodeEvent it keeps the press
     * from moving focus and from placing the caret or selecting text. For
     * a MouseWheelNodeEvent it keeps the wheel from scrolling.
     *
     * Does not affect propagation; use stopPropagation() for that.
     */
    void preventDefault() const;

    /** Returns true once preventDefault() has been called on this event. */
    bool isDefaultPrevented() const;
private:
    Node& _node;
    mutable bool _stopPropagation;
    mutable bool _defaultPrevented;

    friend class Node;
    friend class Document;
};

/** Fired when the left mouse button is pressed and released without dragging.
    The target is the deepest node common to both the press and release hit paths. */
class MouseClickNodeEvent : public NodeEvent {
public:
    /**
     * A click event at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param position  The cursor position in document coordinates.
     * @param modifiers The modifier keys held at the time of the click.
     */
    MouseClickNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the cursor position in document coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held at the time of the click. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/** Fired when the mouse cursor moves over the target node, and once on a node
    that comes under a still cursor because the layout changed. Not fired while
    a mouse button is held; during a left press that hit a node only the drag
    events fire, and during any other hold nothing fires. */
class MouseMoveNodeEvent : public NodeEvent {
public:
    /**
     * A move event at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param position  The new cursor position in document coordinates.
     * @param modifiers The modifier keys held during the move.
     */
    MouseMoveNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the new cursor position in document coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held during the move. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/** Fired for every node that joins the hover path — the newly hit node and any
    of its ancestors not already hovered — whether the cursor moved or the
    layout moved nodes under it. Not fired while a mouse button is held. */
class MouseEnterNodeEvent : public NodeEvent {
public:
    /**
     * An enter event at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param position  The cursor position in document coordinates at entry.
     * @param modifiers The modifier keys held at the time of entry.
     */
    MouseEnterNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the cursor position in document coordinates at entry. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held at the time of entry. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/** Fired for every node that leaves the hover path — previously hovered nodes
    no longer on the hit node's ancestor chain, including when the cursor
    leaves the window. Not fired while a mouse button is held. */
class MouseExitNodeEvent : public NodeEvent {
public:
    /**
     * An exit event at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param position  The cursor position in document coordinates at exit.
     * @param modifiers The modifier keys held at the time of exit.
     */
    MouseExitNodeEvent(Node& node, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns the cursor position in document coordinates at exit. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held at the time of exit. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    KeyModifiers _modifiers;
};

/** Fired when the mouse wheel is scrolled over the target node: the hovered
    node, or the Document when there is none. preventDefault() cancels the
    scroll. */
class MouseWheelNodeEvent : public NodeEvent {
public:
    /**
     * A wheel event with the given scroll delta.
     *
     * @param node      The node that is the target of the event.
     * @param wheel     The scroll delta as (horizontal, vertical) in document units.
     * @param modifiers The modifier keys held during the scroll.
     */
    MouseWheelNodeEvent(Node& node, Vec2 const& wheel, KeyModifiers const& modifiers);

    /** Returns the scroll delta as (horizontal, vertical) in document units. */
    Vec2 const& getWheel() const;

    /** Returns the modifier keys held during the scroll. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _wheel;
    KeyModifiers _modifiers;
};

/** Fired when a mouse button is pressed. Targets the hovered node, or the
    Document when there is none. As on the web, only the left button marks the
    pressed node chain active, and the left and right buttons move focus to it;
    preventDefault() keeps focus where it is and stops the press from placing
    the caret or selecting text. */
class MouseDownNodeEvent : public NodeEvent {
public:
    /**
     * A press event for the given button at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param mouse     The mouse button that was pressed.
     * @param position  The cursor position in document coordinates.
     * @param modifiers  The modifier keys held at the time of the press.
     * @param clickCount The number of consecutive clicks this press is part of (1 for a single click); default 1.
     */
    MouseDownNodeEvent(Node& node, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers, int clickCount = 1);

    /** Returns which mouse button was pressed. */
    Mouse const& getMouse() const;

    /** Returns the cursor position in document coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held at the time of the press. */
    KeyModifiers const& getModifiers() const;

    /** Returns the number of consecutive clicks this press is part of: 1 for a single click, 2 for a double click, and so on. */
    int getClickCount() const;
private:
    Mouse _mouse;
    Vec2 _position;
    KeyModifiers _modifiers;
    int _clickCount;
};

/** Fired when the pressed mouse button is released. Always targets the node
    that received the press — the Document when the press hit nothing — even
    if the cursor has since left it, like pointer capture on the web. */
class MouseUpNodeEvent : public NodeEvent {
public:
    /**
     * A release event for the given button at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param mouse     The mouse button that was released.
     * @param position  The cursor position in document coordinates.
     * @param modifiers The modifier keys held at the time of the release.
     */
    MouseUpNodeEvent(Node& node, Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers);

    /** Returns which mouse button was released. */
    Mouse const& getMouse() const;

    /** Returns the cursor position in document coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the modifier keys held at the time of the release. */
    KeyModifiers const& getModifiers() const;
private:
    Mouse _mouse;
    Vec2 _position;
    KeyModifiers _modifiers;
};

/** Fired when a drag gesture starts on the target node. */
class MouseBeginDragNodeEvent : public NodeEvent {
public:
    /**
     * A drag-start event at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param position  The cursor position in document coordinates at drag start.
     * @param translate The drag translation accumulated since the gesture began.
     * @param modifiers The modifier keys held at drag start.
     */
    MouseBeginDragNodeEvent(Node& node, Vec2 const& position, Vec2 const& translate, KeyModifiers const& modifiers);

    /** Returns the cursor position in document coordinates at drag start. */
    Vec2 const& getPosition() const;

    /** Returns the drag translation accumulated since the gesture began. */
    Vec2 const& getTranslate() const;

    /** Returns the modifier keys held at drag start. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    Vec2 _translate;
    KeyModifiers _modifiers;
};

/** Fired when a drag gesture ends; dispatched on the node that received the press, not the node under the cursor. */
class MouseEndDragNodeEvent : public NodeEvent {
public:
    /**
     * A drag-end event at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param position  The cursor position in document coordinates at drag end.
     * @param translate The total drag translation over the entire gesture.
     * @param modifiers The modifier keys held at drag end.
     */
    MouseEndDragNodeEvent(Node& node, Vec2 const& position, Vec2 const& translate, KeyModifiers const& modifiers);

    /** Returns the cursor position in document coordinates at drag end. */
    Vec2 const& getPosition() const;

    /** Returns the total drag translation over the entire gesture. */
    Vec2 const& getTranslate() const;

    /** Returns the modifier keys held at drag end. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    Vec2 _translate;
    KeyModifiers _modifiers;
};

/** Fired on each cursor movement while a drag gesture is in progress. */
class MouseDragNodeEvent : public NodeEvent {
public:
    /**
     * A drag event at the given position.
     *
     * @param node      The node that is the target of the event.
     * @param position  The current cursor position in document coordinates.
     * @param translate The drag translation accumulated since the gesture began.
     * @param modifiers The modifier keys held during the drag.
     */
    MouseDragNodeEvent(Node& node, Vec2 const& position, Vec2 const& translate, KeyModifiers const& modifiers);

    /** Returns the current cursor position in document coordinates. */
    Vec2 const& getPosition() const;

    /** Returns the drag translation accumulated since the gesture began. */
    Vec2 const& getTranslate() const;

    /** Returns the modifier keys held during the drag. */
    KeyModifiers const& getModifiers() const;
private:
    Vec2 _position;
    Vec2 _translate;
    KeyModifiers _modifiers;
};

/** Fired when a keyboard key is pressed; dispatched on the focused node, or on the Document when nothing is focused or the focused node has key events disabled. */
class KeyDownNodeEvent : public NodeEvent {
public:
    /**
     * A key-press event for the given key.
     *
     * @param node      The node that is the target of the event.
     * @param scancode  The physical key that was pressed.
     * @param keycode   What the key means on the current layout (see Keycode).
     * @param modifiers The modifier keys held at the time of the key press.
     * @param repeat    Whether the key is auto-repeating because it is held down.
     */
    KeyDownNodeEvent(Node& node, Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers, bool repeat = false);

    /** Returns the physical key that was pressed. */
    Scancode getScancode() const;

    /** Returns what the key means on the current layout (see Keycode); use it for shortcuts. */
    std::string const& getKeycode() const;

    /** Returns the modifier keys held at the time of the key press. */
    KeyModifiers const& getModifiers() const;

    /** Returns whether the key is auto-repeating because it is held down. */
    bool isRepeat() const;
private:
    Scancode _scancode;
    std::string _keycode;
    KeyModifiers _modifiers;
    bool _repeat;
};

/** Fired when a keyboard key is released; dispatched on the focused node, or on the Document when nothing is focused or the focused node has key events disabled. */
class KeyUpNodeEvent : public NodeEvent {
public:
    /**
     * A key-release event for the given key.
     *
     * @param node      The node that is the target of the event.
     * @param scancode  The physical key that was released.
     * @param keycode   What the key means on the current layout (see Keycode).
     * @param modifiers The modifier keys held at the time of the key release.
     */
    KeyUpNodeEvent(Node& node, Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers);

    /** Returns the physical key that was released. */
    Scancode getScancode() const;

    /** Returns what the key means on the current layout (see Keycode). */
    std::string const& getKeycode() const;

    /** Returns the modifier keys held at the time of the key release. */
    KeyModifiers const& getModifiers() const;
private:
    Scancode _scancode;
    std::string _keycode;
    KeyModifiers _modifiers;
};

/**
 * Text is about to be inserted into the focused node: typed characters, the
 * result of a dead key or input method, the emoji picker, dictation.
 * Dispatched on the focused editable, or on a focused node with an input
 * area (see Node::setInputArea), which handles the text itself.
 * preventDefault() keeps an editable from inserting it, so a listener can
 * filter or limit what is typed.
 */
class BeforeInputNodeEvent : public NodeEvent {
public:
    /**
     * An event for text about to be inserted.
     *
     * @param node The node that is the target of the event.
     * @param text The UTF-8 text about to be inserted.
     */
    BeforeInputNodeEvent(Node& node, std::string const& text);

    /** Returns the UTF-8 text about to be inserted. */
    std::string const& getText() const;
private:
    std::string _text;
};

/** The node's text content was changed by user editing (see Node::setContentEditable); content is the full new text. */
class InputNodeEvent : public NodeEvent {
public:
    /**
     * An input event with the given content.
     *
     * @param node    The node that is the target of the event.
     * @param content The full new text content of the node.
     */
    InputNodeEvent(Node& node, std::string const& content);

    /** Returns the full new text content of the node. */
    std::string const& getContent() const;
private:
    std::string _content;
};

/** Fired on the node that gains keyboard focus. */
class FocusNodeEvent : public NodeEvent {
public:
    /**
     * A focus event.
     *
     * @param node The node that is the target of the event.
     */
    FocusNodeEvent(Node& node);
};

/** Fired on the node that loses keyboard focus. */
class BlurNodeEvent : public NodeEvent {
public:
    /**
     * A blur event.
     *
     * @param node The node that is the target of the event.
     */
    BlurNodeEvent(Node& node);
};

} /* namespace Rocket */
