/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <map>
#include <chrono>
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
 * (mouse, wheel, keyboard) into node events dispatched through the tree. Call
 * update() to recompute layout and render() to paint the tree into the
 * window. The Document does not own the window and performs no window
 * management of its own.
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
     * Moves focus to the nearest focusable or editable node at or above the
     * given one, blurring whatever held focus before.
     *
     * Walks up from targetNode until it finds a node with a positive tab index
     * or an editable node, and focuses that; if the walk reaches the root
     * without a match, focus is cleared instead. Pass nullptr to blur without
     * focusing anything. Focusing an editable node is what enables its text
     * editing state, and blurring discards the caret and selection.
     *
     * @param targetNode The node to focus from, or nullptr to only blur.
     */
    void focusNode(Node* targetNode);

    /**
     * Recomputes layout for the node tree, resolves the cursor from the hovered
     * node chain, and applies it to the window when it changes.
     *
     * No-op unless the document has been invalidated since the last update.
     * Calling update() re-entrantly (e.g. from an event handler that runs
     * during an update) is a no-op: changes made during an update invalidate
     * the nodes they touch as usual, and the next update() picks them up.
     */
    void update();

    /**
     * Renders the node tree into the window.
     *
     * Call update() beforehand to ensure layout is current. No-op unless a
     * repaint is pending: an update has run, a text-editing mouse
     * interaction changed the caret or selection, or the caret blink phase
     * flipped (the caret of a focused editable blinks at 530ms, restarting
     * visible after every edit or caret move) since the last render.
     */
    void render();

    virtual ~Document();
private:
    struct _InputState {
        Node& boxNode;
        Node& textNode;
        Text& textObject;
    };

    Window& _window;
    Sub<WindowEvent const&> _windowSub;
    Painter _painter;
    Cursor _cursor;
    Node* _hoverNode;
    Node* _activeNode;
    Node* _focusedNode;
    Vec2 _mousePosition;
    Mouse _mouseButton;
    bool _mouseIsDown;
    bool _mouseIsDragging;
    void* _yogaConfig;
    float _scale;
    bool _isUpdating;
    bool _needsUpdate;
    bool _needsRender;
    bool _caretVisible;
    std::chrono::steady_clock::time_point _caretBlinkStart;
    std::vector<Node*> _hoverPath;
    std::map<
        std::int64_t,
        std::vector<Node*>
    > _renderList;

    std::optional<_InputState> _getInputState();
    Node* _getKeyNode();
    Vec2 _getTextLocalPosition(_InputState const&, Vec2 const&) const;
    Node* _findNodeAtPosition(Vec2 const&);
    bool _isNodeFocusable(Node const&) const;
    bool _isNodeEditable(Node const&) const;
    void _focusNext(bool);
    void _restartCaretBlink();
    void _mouseWheel(Vec2 const&, KeyModifiers const&);
    void _mouseMove(Vec2 const&, KeyModifiers const&);
    void _mouseDown(Mouse const&, Vec2 const&, KeyModifiers const&, int);
    void _mouseUp(Mouse const&, Vec2 const&, KeyModifiers const&);
    void _keyDown(Key const&, KeyModifiers const&, std::string const&);
    void _keyUp(Key const&, KeyModifiers const&);
    bool _input(Key const&, KeyModifiers const&, std::string const&);

    void _invalidateNode(Node&);
    void _cascadeNode(Node&);
    void _updateNode(Node&);
    void _renderNode(Node&, Vec2 const&, int);
    void _activateNode(Node*);

    friend class Node;
};

} /* namespace Rocket */
