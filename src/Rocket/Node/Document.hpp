/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <cstdint>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/Enum.hpp>
#include <Rocket/Base/PubSub.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Text.hpp>
#include <Rocket/Window/Window.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Node/Node.hpp>

namespace Rocket {

/**
 * The root of a Node tree, bound to a Window.
 *
 * Constructing a Document registers it as the tree root, sets up layout, and
 * subscribes to the window's event source, translating window input events
 * (mouse, wheel, keyboard) into node events dispatched through the tree,
 * updating and repainting on the window's paint event and relayouting on
 * resize and DPI change. Every window input event updates the document before
 * it returns, so hover, press and focus always reflect the current layout.
 * Call update() after changing the tree outside an event and render() to
 * paint it into the window. The Document does not own the window.
 *
 * Node events that report a state change (enter, exit, move, focus, blur,
 * drag, click and input) are delivered from a queue once the change is
 * complete, in the order they happened; a listener may change the tree,
 * focus or update from inside them. Press, release, wheel and key events are
 * delivered synchronously so a listener can preventDefault() their default
 * action. Like pointer capture on the web, a release always goes to the node
 * that received the press and hover is frozen while a button is held.
 *
 * Non-copyable and non-movable (inherited from Node).
 */
class Document : public Node {
public:
    /**
     * A document bound to the given window.
     *
     * The document subscribes to the window's onEvent source and adopts the
     * window's current scale. The window must outlive the document.
     *
     * @param window The window this document renders into and receives input from.
     */
    Document(Window& window);

    /** Returns the window this document is bound to. */
    Window& getWindow() const;

    /**
     * Returns the size of the document's content area, in points.
     *
     * Computed as (window size × window scale) / document scale, i.e. the
     * window's physical pixel size divided by the document's scale — a bigger
     * scale yields a smaller content size. With the default scale (the
     * window's scale) this equals the window size.
     */
    Vec2 getSize() const;

    /** Returns the scale factor used for layout and rendering. */
    float getScale() const;

    /**
     * Sets the scale factor used for layout and rendering.
     *
     * The document adopts the window's scale on construction; the value set
     * here overrides it. A bigger scale yields a smaller content size (see
     * getSize()).
     *
     * @param scale The new scale factor.
     */
    void setScale(float scale);

    /**
     * Scrolls the nearest scrollable node at or above the given one, per axis, by a wheel delta.
     *
     * The two axes resolve independently: each picks the closest node on the
     * path from the document down to this one that both overflows on that axis
     * and has NodeOverflow::Scroll set for it. Axes with no such node are left
     * alone.
     *
     * @param node  The node the scroll originated from.
     * @param wheel The wheel delta; the scroll position moves opposite to it (content moves with it).
     */
    void scrollNode(Node& node, Vec2 const& wheel);

    /**
     * Scrolls the nearest scrollable ancestor of the given node so the node
     * is fully visible inside it.
     *
     * Walks up from the node's parent to the first ancestor with
     * NodeOverflow::Scroll on either axis and adjusts that container only,
     * each of its scrollable axes independently: the scroll position moves
     * by the least amount that brings the node's border box inside the
     * container's inner border box. A node larger than the container on an
     * axis is aligned to the container's top-left edge on that axis. Uses
     * the layout of the last update(), so call update() first if the tree
     * has changed; the new position takes effect on the next update().
     *
     * @param node The node to bring into view.
     */
    void scrollNodeIntoView(Node& node);

    /**
     * Moves focus to the nearest focusable or editable node at or above the
     * given one, blurring whatever held focus before.
     *
     * Walks up from targetNode until it finds a box node with a positive tab
     * index or an editable node, and focuses that; if the walk reaches the root
     * without a match, focus is cleared instead. Pass nullptr to blur without
     * focusing anything. Focusing an editable node is what enables its text
     * editing state, and blurring discards the caret and selection.
     *
     * @param targetNode The node to focus from, or nullptr to only blur.
     */
    void focusNode(Node* targetNode);

    /**
     * Brings the document up to date: recomputes layout and hover while the
     * tree is invalidated, delivers queued node events, and repeats until
     * neither is pending, then applies the resolved cursor to the window when
     * it changed.
     *
     * Layout is only recomputed when something invalidated it. Calling
     * update() re-entrantly (e.g. from an event listener that runs during an
     * update) is a no-op: the changes it would pick up are handled by the
     * update already running before it returns.
     */
    void update();

    /**
     * Renders the node tree into the window.
     *
     * Call update() beforehand to ensure layout is current. No-op unless a
     * repaint is pending: a layout ran, a paint-only property changed, hover,
     * press or focus changed, the caret or selection of the focused editable
     * moved, or the caret blink phase flipped (the caret blinks at 530ms,
     * restarting visible after every handled key, press or focus change)
     * since the last render.
     */
    void render();

    /**
     * Marks the document for update: the next update() recomputes layout
     * and hover even if nothing in the tree was invalidated. Use it when
     * layout depends on state the document cannot observe.
     */
    void needsUpdate();

    /**
     * Marks the document for render: the next render() repaints even if
     * nothing changed. Use it when what a node paints depends on state the
     * document cannot observe.
     */
    void needsRender();

    virtual ~Document();
private:
    struct _InputState {
        Node& boxNode;
        Node& textNode;
        Text& textObject;
    };

    struct _HoverState {
        Vec2 mousePosition;
        Node* hoverNode;
        std::vector<Node*> hoverPath;
    };

    struct _PressState {
        Node* pressNode;
        std::vector<Node*> pressPath;
    };

    struct _FocusState {
        Node* focusedNode;
        std::vector<Node*> focusedPath;
    };

    struct _DragState {
        Node* dragNode;
        Vec2 mousePosition;
    };

    struct _CaretState {
        bool visible;
        std::int64_t blinkStart;
    };

    struct _MouseState {
        bool down;
        bool inside;
        Mouse mouse;
        Vec2 position;
        Vec2 downPosition;
        KeyModifiers modifiers;
        int clickCount;
        bool defaultPrevented;
    };

    struct _RenderInfo {
        bool empty;
        Vec2 offset;
        QuadShape borderShape;
        Vec4 clipRect;
        std::optional<Vec4> scissorRect;
        std::optional<Vec4> compositeScissor;
        std::optional<Vec4> layerRect;
    };

    struct _NoneFocusNode {};
    struct _PressFocusNode {};
    struct _TargetFocusNode { Node& node; };
    using _FocusNodeVariant = Enum<
        _NoneFocusNode,
        _PressFocusNode,
        _TargetFocusNode
    >;

    struct _HoverScrollNode {};
    struct _TargetScrollNode { Node& node; };
    using _ScrollNodeVariant = Enum<
        _HoverScrollNode,
        _TargetScrollNode
    >;

    Window& _window;
    Sub<WindowEvent const&> _windowSub;
    Cursor _windowCursor;
    Painter _painter;
    _MouseState _mouseState;
    _HoverState _hoverState;
    _PressState _pressState;
    _FocusState _focusState;
    _DragState _dragState;
    _CaretState _caretState;
    void* _config;
    float _scale;
    bool _isUpdating;
    bool _isFlushing;
    bool _isRendering;
    bool _needsUpdate;
    bool _needsRender;
    bool _needsCursor;

    std::map<
        std::int64_t,
        std::vector<Node*>
    > _renderList;

    std::vector<
        std::unique_ptr<NodeEvent>
    > _eventQueue;

    void _handleEvent(WindowEvent const&);
    void _handleMouseMoveEvent(MouseMoveWindowEvent const&);
    void _handleMouseEnterEvent(MouseEnterWindowEvent const&);
    void _handleMouseExitEvent(MouseExitWindowEvent const&);
    void _handleMouseDownEvent(MouseDownWindowEvent const&);
    void _handleMouseUpEvent(MouseUpWindowEvent const&);
    void _handleMouseWheelEvent(MouseWheelWindowEvent const&);
    void _handleKeyDownEvent(KeyDownWindowEvent const&);
    void _handleKeyUpEvent(KeyUpWindowEvent const&);
    void _handleMousePosition(Vec2 const&, KeyModifiers const&, bool);
    void _hoverNode();
    void _pressNode();
    void _releaseNode();
    void _focusNode(_FocusNodeVariant const&);
    void _scrollNode(_ScrollNodeVariant const&, Vec2 const&);
    void _scrollNodeIntoView(Node&);
    void _dragNode();
    void _dropNode();
    void _clickNode();
    void _pressText(bool);
    void _dragText();
    void _releaseText();
    void _focusNext(bool);
    void _processKey(Key, KeyModifiers const&, std::string const&);
    bool _inputText(Key, KeyModifiers const&, std::string const&);
    std::optional<_InputState> _ensureInputState();
    void _restartCaretBlink();
    Vec2 _convertPoint(Vec2 const&) const;
    void _queueEvent(std::unique_ptr<NodeEvent>);
    void _flushEvents();
    void _updateLayout();
    void _updateNode(Node&);
    void _updateCursor();
    void _updateAll();
    void _renderNode(Node&, Vec2 const&, int);
    _RenderInfo _beginNodeRender(Node&, Vec2 const&);
    void _endNodeRender(Node&, _RenderInfo const&);
    void _renderNodeBackground(Node&, _RenderInfo const&);
    void _renderNodeBorder(Node&, _RenderInfo const&);
    void _renderNodeText(Node&, _RenderInfo const&);
    void _renderNodePaint(Node&, _RenderInfo const&);
    void _renderNodeForeground(Node&, _RenderInfo const&);
    void _renderNodeShadow(Node&, _RenderInfo const&, ImageBrush const&);
    void _renderAll();
    void _triggerMouseWheel(Vec2 const&, KeyModifiers const&, bool&);
    void _triggerMouseDown(Mouse, KeyModifiers const&, bool&);
    void _triggerMouseUp(Mouse, KeyModifiers const&);
    void _triggerKeyDown(Key, KeyModifiers const&, std::string const&, bool&);
    void _triggerKeyUp(Key, KeyModifiers const&);
    Vec2 _getTextLocalPosition(_InputState const&, Vec2 const&) const;
    Node* _findNodeAtPosition(Vec2 const&);
    bool _isNodeFocusable(Node const&) const;
    bool _isNodeEditable(Node const&) const;

    friend class Node;
};

} /* namespace Rocket */
