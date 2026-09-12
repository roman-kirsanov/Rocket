/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <optional>
#include <functional>
#include <Rocket/Node/Node.hpp>

namespace Rocket {

/** Base z-index for modal overlays. */
auto constexpr MODAL_ZINDEX = 10;
/** Base z-index for popups (above modals). */
auto constexpr POPUP_ZINDEX = 20;
/** Base z-index for menus (above popups). */
auto constexpr MENU_ZINDEX = 30;
/** Base z-index for tooltips (above menus). */
auto constexpr TOOLTIP_ZINDEX = 40;

/**
 * Declarative props for Node.
 *
 * Each field maps to the matching retained Node setter; a field left unset
 * (std::nullopt) leaves the node at its default. Values are only pushed to
 * the underlying node when they change between renders, so imperative
 * changes made directly on the node are not clobbered by re-renders.
 * Lengths are in points (device-independent units, scaled by the document
 * scale when painted).
 */
struct NodeProps {
    /** When non-null, filled with a pointer to the underlying Node while mounted; reset to nullptr when a different ref is passed, and on unmount unless another Node component has since claimed the same slot (as happens when keyless siblings re-match by ordinal). */
    class Node** ref = nullptr;

    /** Reconciliation key for this component instance (see COMPONENT). */
    std::string key;

    /** When true, parent the node to the Document instead of the nearest Node ancestor. */
    std::optional<bool> root;

    /** Display mode; unset behaves as NodeDisplay::Box. */
    std::optional<NodeDisplay> display;

    /** Horizontal overflow behaviour (hidden / visible / scroll). */
    std::optional<NodeOverflow> overflowX;

    /** Vertical overflow behaviour (hidden / visible / scroll). */
    std::optional<NodeOverflow> overflowY;

    /** Positioning mode: Relative (in flow), Absolute, or Fixed. */
    std::optional<NodePosition> position;

    /** Main-axis direction for children (Horizontal / Vertical, or their Reverse variants). */
    std::optional<NodeDirection> direction;

    /** Cross-axis alignment of children. */
    std::optional<NodeAlignment> alignment;

    /** Cross-axis self-alignment, overriding the parent's alignment for this node. */
    std::optional<NodeAlignment> selfAlignment;

    /** Main-axis distribution of children. */
    std::optional<NodeJustify> justify;

    /** Explicit width. */
    std::optional<NodeValue> width;

    /** Explicit height. */
    std::optional<NodeValue> height;

    /** Minimum width constraint. */
    std::optional<NodeValue> minWidth;

    /** Maximum width constraint. */
    std::optional<NodeValue> maxWidth;

    /** Minimum height constraint. */
    std::optional<NodeValue> minHeight;

    /** Maximum height constraint. */
    std::optional<NodeValue> maxHeight;

    /** Top position offset (used with NodePosition::Absolute or Fixed). */
    std::optional<NodeValue> top;

    /** Left position offset (used with NodePosition::Absolute or Fixed). */
    std::optional<NodeValue> left;

    /** Right position offset (used with NodePosition::Absolute or Fixed). */
    std::optional<NodeValue> right;

    /** Bottom position offset (used with NodePosition::Absolute or Fixed). */
    std::optional<NodeValue> bottom;

    /** Padding for all edges; a per-edge padding overrides it on that edge. */
    std::optional<NodeValue> padding;

    /** Left padding. */
    std::optional<NodeValue> paddingLeft;

    /** Top padding. */
    std::optional<NodeValue> paddingTop;

    /** Right padding. */
    std::optional<NodeValue> paddingRight;

    /** Bottom padding. */
    std::optional<NodeValue> paddingBottom;

    /** Margin for all edges; a per-edge margin overrides it on that edge. */
    std::optional<NodeValue> margin;

    /** Left margin. */
    std::optional<NodeValue> marginLeft;

    /** Top margin. */
    std::optional<NodeValue> marginTop;

    /** Right margin. */
    std::optional<NodeValue> marginRight;

    /** Bottom margin. */
    std::optional<NodeValue> marginBottom;

    /** Gap between children on both axes; a per-axis gap overrides it on that axis. */
    std::optional<NodeValue> gap;

    /** Horizontal gap between children. */
    std::optional<NodeValue> gapX;

    /** Vertical gap between children. */
    std::optional<NodeValue> gapY;

    /** Border width in points for all edges; a per-edge border width overrides it on that edge. */
    std::optional<float> borderWidth;

    /** Left border width in points. */
    std::optional<float> borderLeftWidth;

    /** Top border width in points. */
    std::optional<float> borderTopWidth;

    /** Right border width in points. */
    std::optional<float> borderRightWidth;

    /** Bottom border width in points. */
    std::optional<float> borderBottomWidth;

    /** Corner border radius in points for all corners; a per-corner radius overrides it on that corner. */
    std::optional<float> borderRadius;

    /** Top-left corner border radius in points. */
    std::optional<float> borderTopLeftRadius;

    /** Top-right corner border radius in points. */
    std::optional<float> borderTopRightRadius;

    /** Bottom-left corner border radius in points. */
    std::optional<float> borderBottomLeftRadius;

    /** Bottom-right corner border radius in points. */
    std::optional<float> borderBottomRightRadius;

    /** Focusability: any value > 0 makes the node focusable (0 = not focusable). Does not currently define a traversal order. */
    std::optional<int> tabIndex;

    /** Stacking order; higher paints above lower. */
    std::optional<int> zIndex;

    /** When true (default), the node is clipped to its ancestor's clip rectangle; when false it starts a new clip path. */
    std::optional<bool> clipped;

    /** When true (default), mouse events are delivered to this node. */
    std::optional<bool> mouseEvents;

    /** When true (default), keyboard events are delivered to this node. */
    std::optional<bool> keyEvents;

    /** When true, the node is a text-editing surface: while it is focused, the document edits its first text-display child (see Node::setContentEditable). */
    std::optional<bool> editable;

    /** When true, the node's rendered text is masked with bullets (secure/password display) while the content keeps the real string (see Node::setSecure). */
    std::optional<bool> secure;

    /** When true, the editing surface is multi-line: Enter inserts a line break and Tab inserts spaces. When unset or false the surface is single-line: Enter is not inserted (listen for its KeyDownNodeEvent to submit), Tab is not consumed, and pasted line breaks become spaces (see Node::setContentMultiLine). */
    std::optional<bool> multiLine;

    /** When false, the node and its subtree are neither painted nor hit-tested for mouse events. */
    std::optional<bool> visible;

    /** When true, the node is excluded from layout and text measurement. */
    std::optional<bool> skip;

    /** When true, the node grows and shrinks to fill available main-axis space. */
    std::optional<bool> flex;

    /** Opacity in [0, 1] applied to the node and its subtree. */
    std::optional<float> opacity;

    /** Post-layout translation, in points, applied on top of the computed position. */
    std::optional<Vec2> offset;

    /** Post-layout transform applied on top of the computed position;
     *  translation percentages resolve against the node's own size, as on the
     *  web — e.g. { PercentValue{ -50.0f }, PercentValue{ -100.0f } } positions
     *  the node by its bottom-center. */
    std::optional<NodeTransform> transform;

    /** Mouse cursor shown while hovering this node. */
    std::optional<Cursor> cursor;

    /** Font family name; inherited by descendant text. */
    std::optional<std::string> fontFamily;

    /** Font weight; inherited by descendant text. */
    std::optional<FontWeight> fontWeight;

    /** Font style; inherited by descendant text. */
    std::optional<FontStyle> fontStyle;

    /** Font size in points; inherited by descendant text. */
    std::optional<float> fontSize;

    /** Line height as a unitless multiplier of the font size; inherited by descendant text. */
    std::optional<float> lineHeight;

    /** Text colour (RGBA); inherited by descendant text. */
    std::optional<Vec4> textColor;

    /** Text marker: a background highlight (RGBA) behind this node's own text run; not inherited (see Node::setTextMarker). */
    std::optional<Vec4> textMarker;

    /** The text string to render (used with NodeDisplay::Text). */
    std::optional<std::string> content;

    /** Background brush, painted behind the children. */
    std::optional<Brush> background;

    /** Foreground brush, painted over the children. */
    std::optional<Brush> foreground;

    /** Border brush, painted along the border widths. */
    std::optional<Brush> border;

    /** Drop shadow. */
    std::optional<Shadow> shadow;

    /** Bubbling-phase event handler (target up to root). */
    std::function<void(NodeEvent const&)> onEvent;

    /** Capture-phase event handler (root down to target). */
    std::function<void(NodeEvent const&)> onCaptureEvent;

    /** Children callback invoked before the main children. */
    std::function<void()> prefix;

    /** Children callback invoked after the main children. */
    std::function<void()> suffix;
};

/**
 * Renders a retained Node: mounts one on first render, keeps it in sync
 * with `props` on every render, and unmounts (destroying it) when this call
 * site stops rendering.
 *
 * Must be called during a render (see COMPONENT), with a Document mounted
 * as a context (see Context). The node is parented to the nearest Node
 * ancestor in scope, or to the Document when `props.root` is true or no
 * ancestor exists. Exposes an internal context so nested Node calls (however
 * deeply wrapped in composite components) nest under it in order.
 *
 * @param props    Desired node configuration for this render.
 * @param children Rendered inside the node's context; default no-op.
 */
void Node(NodeProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
