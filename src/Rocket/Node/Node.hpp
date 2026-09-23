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
#include <memory>
#include <optional>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/PubSub.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Shadow.hpp>
#include <Rocket/Paint/Font.hpp>
#include <Rocket/Window/Cursor.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Node/NodeValue.hpp>

namespace Rocket {

/** Controls whether a node is rendered as a flex box or as a text span. */
enum class NodeDisplay {
    /** Flex-box container; participates in Flexbox layout as a box. */
    Box,
    /** Inline text span; content is rendered as shaped text. */
    Text
};

/** Positioning mode applied to the node within its parent layout. */
enum class NodePosition {
    /** Positioned in the normal flow (default). */
    Relative,
    /** Removed from flow; positioned relative to its containing block. */
    Absolute,
    /** Like Absolute, but pinned: ignores the parent's scroll and is excluded from the parent's scrollable area and content box (for popups/overlays). */
    Fixed
};

/** Main axis direction for a flex container's children. */
enum class NodeDirection {
    /** Left to right. */
    Horizontal,
    /** Right to left. */
    HorizontalReverse,
    /** Top to bottom. */
    Vertical,
    /** Bottom to top. */
    VerticalReverse
};

/** Cross-axis alignment of children inside a flex container, or self-alignment within a parent. */
enum class NodeAlignment {
    Start,
    Center,
    Stretch,
    End
};

/** Distribution of children along the main axis of a flex container. */
enum class NodeJustify {
    Start,
    Center,
    End,
    /** Equal space between children; none at edges. */
    SpaceBetween,
    /** Equal space around each child. */
    SpaceAround,
    /** Equal space between children and edges. */
    SpaceEvenly
};

/** Clipping/scrolling behaviour when content overflows the node's bounds. */
enum class NodeOverflow {
    /** Overflow is clipped. */
    Hidden,
    /** Overflow is visible and not clipped. */
    Visible,
    /** Overflow is clipped and the node is scrollable. */
    Scroll
};

/**
 * A post-layout 2D transform applied on top of the node's computed position,
 * following the web's transform semantics: it moves the node visually without
 * affecting the layout of its siblings, and percentages resolve against the
 * node's own size — translate(-50%, -100%) shifts the node left by half its
 * width and up by its full height. Currently translate-only.
 */
struct NodeTransform {
    /** Horizontal translation: pixels, or a percentage of the node's own width. */
    NodeValue translateX = 0.0f;
    /** Vertical translation: pixels, or a percentage of the node's own height. */
    NodeValue translateY = 0.0f;
    bool operator==(NodeTransform const&) const;
    bool operator!=(NodeTransform const&) const;
};

/**
 * A retained-mode UI tree node.
 *
 * Each Node is either a flex-box container (NodeDisplay::Box) or an inline text
 * span (NodeDisplay::Text), determined by its display property. Layout is
 * computed by a flexbox layout engine; text measurement and shaping are
 * handled internally, on demand.
 *
 * Nodes form a doubly-linked tree: every node tracks its parent, first/last
 * child, and previous/next siblings. A node is attached to a Document when it
 * is inserted (directly or transitively) into the Document's tree, and
 * detached when it is removed.
 *
 * Non-copyable and non-movable. The destructor automatically removes the node
 * from its parent and detaches all children before releasing layout and text
 * resources.
 *
 * Events are distributed through two public channels:
 *   - onEvent — bubbling phase: fires from the target up to the root.
 *   - onCaptureEvent — capture phase: fires from the root down to the target.
 */
class Node {
public:
    /** Bubbling-phase event channel. Fires on this node after the capture
     *  phase (even when a capture handler stopped propagation), then propagates
     *  upward to the root. */
    Pub<NodeEvent const&> onEvent;

    /** Capture-phase event channel. Fires on this node during the capture walk
     *  from the root down to the dispatch target (ancestors first, target last),
     *  before the bubbling phase begins. */
    Pub<NodeEvent const&> onCaptureEvent;

    Node();
    Node(Node &&) = delete;
    Node(Node const&) = delete;
    Node& operator=(Node &&) = delete;
    Node& operator=(Node const&) = delete;

    /** Returns the Document this node is currently attached to, or nullptr if detached. */
    class Document* getDocument() const;

    /** Returns the parent node, or nullptr if this is a root node. */
    Node* getParent() const;

    /** Returns the first child node, or nullptr if this node has no children. */
    Node* getFirstChild() const;

    /** Returns the last child node, or nullptr if this node has no children. */
    Node* getLastChild() const;

    /** Returns the previous sibling, or nullptr if this is the first child. */
    Node* getPrevSibling() const;

    /** Returns the next sibling, or nullptr if this is the last child. */
    Node* getNextSibling() const;

    /** Returns the most recently computed border-box rectangle, in document coordinates (ancestor scroll applied). */
    Vec4 const& getComputedBorderRect() const;

    /** Returns the most recently computed margin-box rectangle, in document coordinates (ancestor scroll applied). */
    Vec4 const& getComputedMarginRect() const;

    /** Returns the most recently computed clip rectangle that applies to this node's children, in document coordinates; the node's own box is clipped by its parent's. */
    Vec4 const& getComputedClipRect() const;

    /** Returns the most recently computed z-index. */
    std::int64_t getComputedZIndex() const;

    /** Returns the most recently computed font family, inherited from ancestors ("" = the embedded default face). */
    std::string const& getComputedFontFamily() const;

    /** Returns the most recently computed font weight, inherited from ancestors. */
    FontWeight getComputedFontWeight() const;

    /** Returns the most recently computed font style, inherited from ancestors. */
    FontStyle getComputedFontStyle() const;

    /** Returns the most recently computed font size in pixels, inherited from ancestors. */
    float getComputedFontSize() const;

    /** Returns the most recently computed text color, inherited from ancestors. */
    Vec4 const& getComputedTextColor() const;

    /** Returns the text marker color this node's text is drawn with after the last update: its own (alpha 0 when unset), since markers are not inherited. */
    Vec4 const& getComputedTextMarker() const;

    /** Returns the most recently computed line height, inherited from ancestors, as a unitless multiplier of the font size. */
    float getComputedLineHeight() const;

    /** Returns the display mode if set, otherwise std::nullopt. */
    std::optional<NodeDisplay> const& getDisplay() const;

    /** Returns the main-axis direction if set, otherwise std::nullopt. */
    std::optional<NodeDirection> const& getDirection() const;

    /** Returns the cross-axis alignment of children if set, otherwise std::nullopt. */
    std::optional<NodeAlignment> const& getAlignment() const;

    /** Returns the main-axis justification of children if set, otherwise std::nullopt. */
    std::optional<NodeJustify> const& getJustify() const;

    /** Returns the horizontal overflow mode if set, otherwise std::nullopt. */
    std::optional<NodeOverflow> const& getOverflowX() const;

    /** Returns the vertical overflow mode if set, otherwise std::nullopt. */
    std::optional<NodeOverflow> const& getOverflowY() const;

    /** Returns the positioning mode if set, otherwise std::nullopt. */
    std::optional<NodePosition> const& getPosition() const;

    /** Returns the self cross-axis alignment override if set, otherwise std::nullopt. */
    std::optional<NodeAlignment> const& getSelfAlignment() const;

    /** Returns the explicit width if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getWidth() const;

    /** Returns the explicit height if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getHeight() const;

    /** Returns the minimum width if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMinWidth() const;

    /** Returns the maximum width if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMaxWidth() const;

    /** Returns the minimum height if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMinHeight() const;

    /** Returns the maximum height if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMaxHeight() const;

    /** Returns the top position offset if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getTop() const;

    /** Returns the left position offset if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getLeft() const;

    /** Returns the right position offset if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getRight() const;

    /** Returns the bottom position offset if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getBottom() const;

    /** Returns the all-edge padding if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getPadding() const;

    /** Returns the top padding if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getPaddingTop() const;

    /** Returns the left padding if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getPaddingLeft() const;

    /** Returns the right padding if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getPaddingRight() const;

    /** Returns the bottom padding if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getPaddingBottom() const;

    /** Returns the all-edge margin if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMargin() const;

    /** Returns the top margin if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMarginTop() const;

    /** Returns the left margin if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMarginLeft() const;

    /** Returns the right margin if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMarginRight() const;

    /** Returns the bottom margin if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getMarginBottom() const;

    /** Returns the all-axis gap between children if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getGap() const;

    /** Returns the horizontal gap between children if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getGapX() const;

    /** Returns the vertical gap between children if set, otherwise std::nullopt. */
    std::optional<NodeValue> const& getGapY() const;

    /** Returns the all-edge border width in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderWidth() const;

    /** Returns the top border width in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderTopWidth() const;

    /** Returns the left border width in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderLeftWidth() const;

    /** Returns the right border width in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderRightWidth() const;

    /** Returns the bottom border width in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderBottomWidth() const;

    /** Returns the all-corner border radius in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderRadius() const;

    /** Returns the top-left corner border radius in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderTopLeftRadius() const;

    /** Returns the top-right corner border radius in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderTopRightRadius() const;

    /** Returns the bottom-left corner border radius in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderBottomLeftRadius() const;

    /** Returns the bottom-right corner border radius in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getBorderBottomRightRadius() const;

    /** Returns the visibility flag if set, otherwise std::nullopt. */
    std::optional<bool> const& getVisible() const;

    /** Returns the z-index if set, otherwise std::nullopt. */
    std::optional<int> const& getZIndex() const;

    /** Returns the post-layout translation offset if set, otherwise std::nullopt. */
    std::optional<Vec2> const& getOffset() const;

    /** Returns the opacity (0.0–1.0) if set, otherwise std::nullopt. */
    std::optional<float> const& getOpacity() const;

    /** Returns the post-layout transform if set, otherwise std::nullopt. */
    std::optional<NodeTransform> const& getTransform() const;

    /** Returns the font family name if set, otherwise std::nullopt. */
    std::optional<std::string> const& getFontFamily() const;

    /** Returns the font weight if set, otherwise std::nullopt. */
    std::optional<FontWeight> const& getFontWeight() const;

    /** Returns the font style if set, otherwise std::nullopt. */
    std::optional<FontStyle> const& getFontStyle() const;

    /** Returns the font size in pixels if set, otherwise std::nullopt. */
    std::optional<float> const& getFontSize() const;

    /** Returns the line height (a unitless multiplier of the font size) if set, otherwise std::nullopt.
     *  Note: this value is stored but not currently consumed by the text engine. */
    std::optional<float> const& getLineHeight() const;

    /** Returns the text color (RGBA) if set, otherwise std::nullopt. */
    std::optional<Vec4> const& getTextColor() const;

    /** Returns the text marker (background highlight, RGBA) if set, otherwise std::nullopt. */
    std::optional<Vec4> const& getTextMarker() const;

    /** Returns the background brush if set, otherwise std::nullopt. */
    std::optional<Brush> const& getBackground() const;

    /** Returns the foreground brush if set, otherwise std::nullopt. */
    std::optional<Brush> const& getForeground() const;

    /** Returns the border brush if set, otherwise std::nullopt. */
    std::optional<Brush> const& getBorder() const;

    /** Returns the drop shadow if set, otherwise std::nullopt. */
    std::optional<Shadow> const& getShadow() const;

    /** Returns the mouse cursor shape if set, otherwise std::nullopt. */
    std::optional<Cursor> const& getCursor() const;

    /** Returns the text content string if set, otherwise std::nullopt. */
    std::optional<std::string> const& getContent() const;

    /** Returns whether text editing is enabled on this node. */
    bool getContentEditable() const;

    /** Returns whether secure text display (bullet masking) is enabled on this node. */
    bool getContentSecure() const;

    /** Returns whether multi-line editing mode is enabled on this node. */
    bool getContentMultiLine() const;

    /** Returns the current value of the skip flag. */
    bool getSkip() const;

    /** Returns the current value of the flex flag. */
    bool getFlex() const;

    /** Returns whether this node receives key events when focused (default true). */
    bool getKeyEvents() const;

    /** Returns whether mouse hit-testing is enabled for this node and its subtree. */
    bool getMouseEvents() const;

    /** Returns whether the node is clipped to its ancestor clip rectangle. */
    bool getClipped() const;

    /** Returns the focusability index; any value > 0 makes a box node focusable (editable boxes are focusable regardless). 0 by default. */
    int getTabIndex() const;

    /** Returns true if the mouse pointer is currently over this node or a descendant. Not updated while a mouse button is held. */
    bool isHover() const;

    /** Returns true if this node or a descendant is currently being pressed with the left mouse button. */
    bool isActive() const;

    /** Returns true if this node has keyboard focus. */
    bool isFocused() const;

    /** Returns true if this node or any descendant has keyboard focus. */
    bool isFocusedWithin() const;

    /**
     * Fills path with the ordered ancestor chain from the root down to this node.
     *
     * Clears path first, then appends nodes from the root (index 0) to this
     * node (last index).
     */
    void getPath(std::vector<Node*>&) const;

    /**
     * Sets the display mode.
     *
     * Passing std::nullopt clears the value. Triggers layout recomputation and
     * text invalidation because changing display affects whether a text
     * measurement node is created or destroyed.
     */
    void setDisplay(std::optional<NodeDisplay> const&);

    /**
     * Sets the main-axis direction of the flex container.
     *
     * Passing std::nullopt resets to the default (Horizontal).
     */
    void setDirection(std::optional<NodeDirection> const&);

    /**
     * Sets the cross-axis alignment of children.
     *
     * Passing std::nullopt resets to the default (Start).
     */
    void setAlignment(std::optional<NodeAlignment> const&);

    /**
     * Sets the main-axis justification of children.
     *
     * Passing std::nullopt resets to the default (Start).
     */
    void setJustify(std::optional<NodeJustify> const&);

    /** Sets the horizontal overflow mode. Passing std::nullopt clears the value. */
    void setOverflowX(std::optional<NodeOverflow> const&);

    /** Sets the vertical overflow mode. Passing std::nullopt clears the value. */
    void setOverflowY(std::optional<NodeOverflow> const&);

    /**
     * Sets the positioning mode.
     *
     * Passing std::nullopt resets to Relative positioning.
     */
    void setPosition(std::optional<NodePosition> const&);

    /**
     * Sets the self cross-axis alignment, overriding the parent's alignment setting.
     *
     * Passing std::nullopt resets to Auto (defers to parent).
     */
    void setSelfAlignment(std::optional<NodeAlignment> const&);

    /** Sets the explicit width. Passing std::nullopt clears the width constraint. */
    void setWidth(std::optional<NodeValue> const&);

    /** Sets the explicit height. Passing std::nullopt clears the height constraint. */
    void setHeight(std::optional<NodeValue> const&);

    /** Sets the minimum width. Passing std::nullopt clears the constraint. */
    void setMinWidth(std::optional<NodeValue> const&);

    /** Sets the maximum width. Passing std::nullopt clears the constraint. */
    void setMaxWidth(std::optional<NodeValue> const&);

    /** Sets the minimum height. Passing std::nullopt clears the constraint. */
    void setMinHeight(std::optional<NodeValue> const&);

    /** Sets the maximum height. Passing std::nullopt clears the constraint. */
    void setMaxHeight(std::optional<NodeValue> const&);

    /** Sets the top position offset. Passing std::nullopt clears the position. */
    void setTop(std::optional<NodeValue> const&);

    /** Sets the left position offset. Passing std::nullopt clears the position. */
    void setLeft(std::optional<NodeValue> const&);

    /** Sets the right position offset. Passing std::nullopt clears the position. */
    void setRight(std::optional<NodeValue> const&);

    /** Sets the bottom position offset. Passing std::nullopt clears the position. */
    void setBottom(std::optional<NodeValue> const&);

    /** Sets the padding for all edges; a per-edge padding, when set, overrides it on that edge.
     *  Passing std::nullopt clears the padding. */
    void setPadding(std::optional<NodeValue> const&);

    /** Sets the top padding. Passing std::nullopt reverts the edge to the shorthand padding, if any. */
    void setPaddingTop(std::optional<NodeValue> const&);

    /** Sets the left padding. Passing std::nullopt reverts the edge to the shorthand padding, if any. */
    void setPaddingLeft(std::optional<NodeValue> const&);

    /** Sets the right padding. Passing std::nullopt reverts the edge to the shorthand padding, if any. */
    void setPaddingRight(std::optional<NodeValue> const&);

    /** Sets the bottom padding. Passing std::nullopt reverts the edge to the shorthand padding, if any. */
    void setPaddingBottom(std::optional<NodeValue> const&);

    /** Sets the margin for all edges; a per-edge margin, when set, overrides it on that edge.
     *  Passing std::nullopt clears the margin. */
    void setMargin(std::optional<NodeValue> const&);

    /** Sets the top margin. Passing std::nullopt reverts the edge to the shorthand margin, if any. */
    void setMarginTop(std::optional<NodeValue> const&);

    /** Sets the left margin. Passing std::nullopt reverts the edge to the shorthand margin, if any. */
    void setMarginLeft(std::optional<NodeValue> const&);

    /** Sets the right margin. Passing std::nullopt reverts the edge to the shorthand margin, if any. */
    void setMarginRight(std::optional<NodeValue> const&);

    /** Sets the bottom margin. Passing std::nullopt reverts the edge to the shorthand margin, if any. */
    void setMarginBottom(std::optional<NodeValue> const&);

    /** Sets the gap between children on both axes; a per-axis gap, when set, overrides it on that axis.
     *  Passing std::nullopt clears the gap. */
    void setGap(std::optional<NodeValue> const&);

    /** Sets the horizontal (column) gap between children. Passing std::nullopt reverts to the shorthand gap, if any. */
    void setGapX(std::optional<NodeValue> const&);

    /** Sets the vertical (row) gap between children. Passing std::nullopt reverts to the shorthand gap, if any. */
    void setGapY(std::optional<NodeValue> const&);

    /** Sets the border width in pixels for all edges; a per-edge border width, when set, overrides it on that edge.
     *  Passing std::nullopt clears the border. */
    void setBorderWidth(std::optional<float> const&);

    /** Sets the top border width in pixels. Passing std::nullopt reverts the edge to the shorthand width, if any. */
    void setBorderTopWidth(std::optional<float> const&);

    /** Sets the left border width in pixels. Passing std::nullopt reverts the edge to the shorthand width, if any. */
    void setBorderLeftWidth(std::optional<float> const&);

    /** Sets the right border width in pixels. Passing std::nullopt reverts the edge to the shorthand width, if any. */
    void setBorderRightWidth(std::optional<float> const&);

    /** Sets the bottom border width in pixels. Passing std::nullopt reverts the edge to the shorthand width, if any. */
    void setBorderBottomWidth(std::optional<float> const&);

    /** Sets the corner border radius in pixels for all corners; a per-corner radius, when set, overrides it on that corner.
     *  Passing std::nullopt clears the radius. */
    void setBorderRadius(std::optional<float> const&);

    /** Sets the top-left corner border radius in pixels. Passing std::nullopt reverts the corner to the shorthand radius, if any. */
    void setBorderTopLeftRadius(std::optional<float> const&);

    /** Sets the top-right corner border radius in pixels. Passing std::nullopt reverts the corner to the shorthand radius, if any. */
    void setBorderTopRightRadius(std::optional<float> const&);

    /** Sets the bottom-left corner border radius in pixels. Passing std::nullopt reverts the corner to the shorthand radius, if any. */
    void setBorderBottomLeftRadius(std::optional<float> const&);

    /** Sets the bottom-right corner border radius in pixels. Passing std::nullopt reverts the corner to the shorthand radius, if any. */
    void setBorderBottomRightRadius(std::optional<float> const&);

    /** Sets the visibility flag. Passing std::nullopt clears the value. */
    void setVisible(std::optional<bool> const&);

    /** Sets the z-index. Passing std::nullopt clears the value. */
    void setZIndex(std::optional<int> const&);

    /** Sets a post-layout pixel translation applied on top of the computed position.
     *  Passing std::nullopt clears the offset. */
    void setOffset(std::optional<Vec2> const&);

    /** Sets the opacity (0.0 = fully transparent, 1.0 = fully opaque).
     *  Passing std::nullopt clears the value. */
    void setOpacity(std::optional<float> const&);

    /** Sets a post-layout transform applied on top of the computed position;
     *  translation percentages resolve against the node's own size, as on the
     *  web. Passing std::nullopt clears the transform. */
    void setTransform(std::optional<NodeTransform> const&);

    /** Sets the font family name. Passing std::nullopt clears the value and invalidates text. */
    void setFontFamily(std::optional<std::string> const&);

    /** Sets the font weight. Passing std::nullopt clears the value and invalidates text. */
    void setFontWeight(std::optional<FontWeight> const&);

    /** Sets the font style. Passing std::nullopt clears the value and invalidates text. */
    void setFontStyle(std::optional<FontStyle> const&);

    /** Sets the font size in pixels. Passing std::nullopt clears the value and invalidates text. */
    void setFontSize(std::optional<float> const&);

    /** Sets the line height value. Stored and cascaded but not currently consumed by the text engine. Passing std::nullopt clears the value and invalidates text. */
    void setLineHeight(std::optional<float> const&);

    /** Sets the text color (RGBA). Passing std::nullopt clears the value and invalidates text. */
    void setTextColor(std::optional<Vec4> const&);

    /**
     * Sets the text marker: a background highlight (RGBA) painted behind
     * this node's own text run, as a search-hit mark or a selection would
     * be. Unlike the other text properties it is not inherited, so a marked
     * span inside a text node marks only its own characters. Passing
     * std::nullopt clears the value and invalidates text.
     */
    void setTextMarker(std::optional<Vec4> const&);

    /** Sets the background brush. Passing std::nullopt clears the value. */
    void setBackground(std::optional<Brush> const&);

    /** Sets the foreground brush. Passing std::nullopt clears the value. */
    void setForeground(std::optional<Brush> const&);

    /** Sets the border brush used to paint the border widths. Passing std::nullopt clears the value. */
    void setBorder(std::optional<Brush> const&);

    /** Sets the drop shadow. Passing std::nullopt clears the value. */
    void setShadow(std::optional<Shadow> const&);

    /** Sets the mouse cursor shape shown while hovering this node. Passing std::nullopt clears the value. */
    void setCursor(std::optional<Cursor> const&);

    /** Sets the text content string. Passing std::nullopt clears the value and invalidates text. */
    void setContent(std::optional<std::string> const&);

    /**
     * Marks this node as a text-editing surface. Only effective on a box node
     * whose first text-display child holds the edited content; while this box
     * is focused, the document handles caret, selection, and content mutation
     * on that child, and left presses inside the box drive the editor unless
     * they land on a focusable descendant.
     */
    void setContentEditable(bool);

    /**
     * Enables secure text display: every codepoint of the rendered text is
     * masked with a bullet (U+2022) while the content keeps the real string
     * (see Text::setSecure). Effective on the text-display node itself or,
     * for an editable field, on the editable box; either one masks the text
     * and blocks copy and cut.
     */
    void setContentSecure(bool);

    /**
     * Enables multi-line editing mode on this node (see setContentEditable):
     * Enter inserts a line break, Tab inserts spaces, pasted and typed line
     * breaks are kept (normalised to \n), and ArrowUp/ArrowDown move between
     * lines. When false (the default) the surface is single-line: Enter is
     * not inserted (listen for its KeyDownNodeEvent to submit), Tab moves
     * focus to the next focusable node (Shift+Tab to the previous), line
     * breaks are replaced with spaces, the text never wraps and scrolls
     * horizontally to keep the caret in view, and ArrowUp/ArrowDown move to
     * the line start/end. Content set programmatically via setContent is not
     * sanitized; keeping single-line content free of line breaks is the
     * caller's contract.
     */
    void setContentMultiLine(bool);

    /** Sets the focusability index: any value > 0 makes a box node focusable
     *  (text nodes are never focusable; editable boxes always are). The value
     *  does not currently define a traversal order. */
    void setTabIndex(int);

    /** When true, this node is excluded from layout and text measurement. */
    void setSkip(bool);

    /**
     * When true, the node grows and shrinks to fill available space on the
     * main axis (flex-grow 1, flex-shrink 1, flex-basis 0, as React
     * Native's `flex: 1`). Sibling flex nodes share the space equally,
     * whatever their content. The parent must have a definite main size
     * (set, stretched, or flex itself): inside a content-sized parent a
     * flex node has nothing to fill and collapses to zero.
     */
    void setFlex(bool);

    /** When true (default), the node receives key events while focused. When
     *  false, a focused node emits no KeyDownNodeEvent, KeyUpNodeEvent or
     *  InputNodeEvent and its text is not edited; the key events are
     *  dispatched on the document instead, and Tab still moves focus. */
    void setKeyEvents(bool);

    /** When true (default), this node is mouse hit-testable; when false, the
     *  node and its whole subtree are excluded from mouse hit-testing. */
    void setMouseEvents(bool);

    /** When true (default), the node is clipped to its parent's clip rectangle; when false it starts a new clip path. */
    void setClipped(bool);

    /**
     * Returns true if node is this node or a descendant of this node.
     *
     * Walks node's ancestor chain upward; returns true as soon as this node
     * is encountered.
     */
    bool containsNode(Node const&);

    /** Appends this node to parent, equivalent to parent.appendChild(*this). */
    void setParent(Node&);

    /**
     * Appends child as the last child of this node; equivalent to
     * insertChild(child, <child count>).
     *
     * If child already has a parent it is first removed from that parent.
     * No-ops if child is this node or if child is an ancestor of this node
     * (to prevent cycles). Propagates the current Document attachment to child
     * and its subtree, then triggers a layout and text invalidation pass.
     */
    void appendChild(Node&);

    /**
     * Inserts child at position index among this node's children, so that
     * afterwards it is the index-th child (0 = first). An index at or past
     * the current child count appends; a negative index inserts first.
     *
     * If child already has a parent it is first removed from that parent;
     * when that parent is this node, index refers to the child list without
     * it, and a child already at index is left untouched. No-ops if child is
     * this node or an ancestor of this node. Propagates the current Document
     * attachment to child and its subtree, then triggers a layout and text
     * invalidation pass.
     *
     * @param child The node to insert.
     * @param index The position among the children to insert at.
     */
    void insertChild(Node& child, std::int64_t index);

    /**
     * Removes child from this node's child list.
     *
     * No-ops if child is not a direct child of this node. Detaches child
     * and its subtree from the current Document and triggers layout/text
     * invalidation on this node.
     */
    void removeChild(Node&);

    /** Removes this node from its parent. No-ops if this node has no parent. */
    void removeFromParent();

    /** Removes all direct children from this node. */
    void removeAllChildren();

    /**
     * Fires event directly on this node only.
     *
     * If capture is false (default) the event is published on onEvent
     * (bubbling channel). If capture is true it is published on
     * onCaptureEvent (capture channel). No propagation occurs.
     */
    void triggerEvent(NodeEvent const&, bool = false);

    /**
     * Dispatches event along the full ancestor path of this node.
     *
     * First performs the capture phase: walks from the root down to this node,
     * calling triggerEvent(..., true) on each ancestor and finally on this
     * node. Then performs the bubbling phase: calls triggerEvent(..., false)
     * on this node and walks upward to the root. stopPropagation cuts the
     * capture phase short immediately; in the bubbling phase this node is
     * always notified first and only the remaining ancestors are skipped.
     */
    void dispatchEvent(NodeEvent const&);

    /**
     * Fires event on this node and recursively on every descendant.
     *
     * Events are delivered in depth-first, pre-order: this node first, then
     * each child subtree in sibling order. Only the bubbling channel
     * (triggerEvent with capture = false) is used.
     */
    void broadcastEvent(NodeEvent const&);

    virtual ~Node();
private:
    Document* _document;
    Node* _parent;
    Node* _firstChild;
    Node* _lastChild;
    Node* _nextSibling;
    Node* _prevSibling;
    std::optional<NodeDisplay> _display;
    std::optional<NodeOverflow> _overflowX;
    std::optional<NodeOverflow> _overflowY;
    std::optional<NodePosition> _position;
    std::optional<NodeDirection> _direction;
    std::optional<NodeAlignment> _alignment;
    std::optional<NodeJustify> _justify;
    std::optional<NodeAlignment> _selfAlignment;
    std::optional<NodeValue> _width;
    std::optional<NodeValue> _height;
    std::optional<NodeValue> _minWidth;
    std::optional<NodeValue> _maxWidth;
    std::optional<NodeValue> _minHeight;
    std::optional<NodeValue> _maxHeight;
    std::optional<NodeValue> _top;
    std::optional<NodeValue> _left;
    std::optional<NodeValue> _right;
    std::optional<NodeValue> _bottom;
    std::optional<NodeValue> _padding;
    std::optional<NodeValue> _paddingTop;
    std::optional<NodeValue> _paddingLeft;
    std::optional<NodeValue> _paddingRight;
    std::optional<NodeValue> _paddingBottom;
    std::optional<NodeValue> _margin;
    std::optional<NodeValue> _marginTop;
    std::optional<NodeValue> _marginLeft;
    std::optional<NodeValue> _marginRight;
    std::optional<NodeValue> _marginBottom;
    std::optional<NodeValue> _gap;
    std::optional<NodeValue> _gapX;
    std::optional<NodeValue> _gapY;
    std::optional<float> _borderWidth;
    std::optional<float> _borderTopWidth;
    std::optional<float> _borderLeftWidth;
    std::optional<float> _borderRightWidth;
    std::optional<float> _borderBottomWidth;
    std::optional<float> _borderRadius;
    std::optional<float> _borderTopLeftRadius;
    std::optional<float> _borderTopRightRadius;
    std::optional<float> _borderBottomLeftRadius;
    std::optional<float> _borderBottomRightRadius;
    std::optional<bool> _visible;
    std::optional<int> _zIndex;
    std::optional<Vec2> _offset;
    std::optional<float> _opacity;
    std::optional<NodeTransform> _transform;
    std::optional<std::string> _fontFamily;
    std::optional<FontWeight> _fontWeight;
    std::optional<FontStyle> _fontStyle;
    std::optional<float> _fontSize;
    std::optional<float> _lineHeight;
    std::optional<Vec4> _textColor;
    std::optional<Vec4> _textMarker;
    std::optional<Brush> _background;
    std::optional<Brush> _foreground;
    std::optional<Brush> _border;
    std::optional<Shadow> _shadow;
    std::optional<Cursor> _cursor;
    std::optional<std::string> _content;
    bool _contentEditable;
    bool _contentSecure;
    bool _contentMultiLine;
    int _tabIndex;
    bool _skip;
    bool _flex;
    bool _keyEvents;
    bool _mouseEvents;
    bool _clipped;
    bool _isHover;
    bool _isActive;
    bool _isFocused;
    bool _isFocusedWithin;
    float _textScrollX;
    std::unique_ptr<class Text> _textObject;
    std::string _computedFontFamily;
    FontWeight _computedFontWeight;
    FontStyle _computedFontStyle;
    float _computedFontSize;
    float _computedLineHeight;
    Vec4 _computedTextColor;
    Vec4 _computedMarkerColor;
    Vec4 _computedTextRect;
    Vec4 _computedBorderEdge;
    Vec4 _computedBorderRect;
    Vec4 _computedMarginRect;
    Vec4 _computedContentRect;
    Vec4 _computedBorderRectInDocument;
    Vec4 _computedMarginRectInDocument;
    Vec4 _computedClipRectInDocument;
    std::int64_t _computedZIndex;
    std::unique_ptr<Image> _layerImage;
    std::unique_ptr<Image> _shadowImage;
    std::optional<Shadow> _shadowImageShadow;
    std::optional<Vec4> _shadowImageRadius;
    std::optional<Vec4> _shadowImageClipRect;
    std::optional<float> _shadowImageScale;
    Vec2 _scrollOverflow;
    Vec2 _scrollPosition;

    void* _layoutNode;
    void* _textNode;

    bool _needsTextUpdate;
    bool _needsLayoutUpdate;

    void _setNodePadding();
    void _setNodeMargin();
    void _setNodeGap();
    void _setNodeBorder();
    void _resetTextState();
    void _resetLayoutState();
    void _resetPaintState();
    void _resetInputState();
    void _attach(Document&);
    void _detach();
    void _updateLayout();
    void _createTextNode();
    void _destroyTextNode();

    friend class Document;
};

} /* namespace Rocket */
