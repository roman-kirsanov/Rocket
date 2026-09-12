/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <string>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Which sibling a Splitter resizes: the previous or the next one. */
enum class SplitterTarget {
    Prev,
    Next
};

/** Drag axis of a Splitter: Horizontal resizes width, Vertical resizes height. */
enum class SplitterDirection {
    Horizontal,
    Vertical
};

/** Props for the Splitter component (a draggable divider between two siblings). */
struct SplitterProps {
    /** Reconciliation key. */
    std::string key;
    /** Sibling to resize while dragging (default Prev). */
    std::optional<SplitterTarget> target;
    /** Drag axis (default Horizontal). */
    std::optional<SplitterDirection> direction;
    /** Splitter active color */
    std::optional<Vec4> activeColor;
    /** Splitter color */
    std::optional<Vec4> color;
    /** Props forwarded to the underlying Node. */
    NodeProps nodeProps;
};

/**
 * Renders a draggable divider that resizes its target sibling by setting that
 * node's width (Horizontal) or height (Vertical) during the drag, clamped at
 * zero. Shows a resize cursor and a highlight while hovered or dragging.
 *
 * @param props Desired splitter configuration for this render.
 */
void Splitter(SplitterProps const& props);

} /* namespace Rocket */
