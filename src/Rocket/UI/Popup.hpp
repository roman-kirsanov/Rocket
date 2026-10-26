/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** How a popup keeps inside the document when it would run past an edge. */
enum class PopupCollision {
    /** Left where it is placed, even past an edge. */
    None,
    /** Slid back inside the document on an axis where it runs past an edge. */
    Shift,
    /** Mirrored to the other side of its anchor on an axis where it runs past an edge, when it runs past less there; never slid. */
    Flip,
    /** Its maximum width and height capped to the document, so its content scrolls, then shifted as by Shift. */
    Fit
};

/** The axes a popup is mirrored on to keep inside the document. */
struct PopupFlip {
    /** True when the popup is mirrored above or below its anchor. */
    bool vertical = false;
    /** True when the popup is mirrored to the left or right of its anchor. */
    bool horizontal = false;
};

/** Vertical edge of the anchor the popup is positioned from. */
enum class PopupVerticalOrigin {
    Top,
    Center,
    Bottom
};

/** Horizontal edge of the anchor the popup is positioned from. */
enum class PopupHorizontalOrigin {
    Left,
    Center,
    Right
};

/** A vertical + horizontal origin pair. */
struct PopupOrigin {
    PopupVerticalOrigin vertical;
    PopupHorizontalOrigin horizontal;
};

/** Props for the Popup component (an absolutely-positioned overlay). */
struct PopupProps {
    /** Reconciliation key. */
    std::string key;
    /** Which edge of the parent the popup is anchored to (default top-left). */
    std::optional<PopupOrigin> anchorOrigin;
    /** Which point of the popup aligns to the anchor (default top-left). */
    std::optional<PopupOrigin> transformOrigin;
    /** How the popup keeps inside the document; unless None, it stays hidden for the render before it can be measured (default Shift). */
    std::optional<PopupCollision> collision;
    /** Gap, in points, between the anchor point and the popup; mirrored on an axis the popup is flipped on (default none). */
    std::optional<Vec2> offset;
    /** Invoked on every render, before `children` are rendered, with the axes the popup is flipped on. */
    std::function<void(PopupFlip const&)> onFlip;
    /** Invoked when a press occurs outside the popup's parent. */
    std::function<void()> onClose;
    /** Renders content pinned above the scrolling content. */
    std::function<void()> header;
    /** Renders content pinned below the scrolling content. */
    std::function<void()> footer;
    /** Props forwarded to the scrolling content Node that holds `children`. */
    NodeProps contentProps;
    /** Props forwarded to the underlying Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders an absolutely-positioned overlay with a themed background, anchored
 * relative to its parent: an optional header, a scrolling content area
 * holding `children`, and an optional footer. Calls `onClose` when a press
 * lands outside its parent. By `collision`, the popup is kept inside the
 * document: it is measured from the layout of the last update and then
 * shifted by however far it runs past an edge, or mirrored to the other side
 * of its anchor, so the correction follows the anchor one render behind. An
 * axis on which `nodeProps` sets both edges is never mirrored; one on which
 * it sets one edge is mirrored around that point.
 *
 * @param props    Desired popup configuration for this render.
 * @param children Rendered inside the popup; default no-op.
 */
void Popup(PopupProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
