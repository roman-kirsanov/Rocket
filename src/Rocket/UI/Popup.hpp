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
 * lands outside its parent.
 *
 * @param props    Desired popup configuration for this render.
 * @param children Rendered inside the popup; default no-op.
 */
void Popup(PopupProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
