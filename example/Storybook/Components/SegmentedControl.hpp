/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <Rocket/UI/Node.hpp>

namespace Storybook {

/** Props for the SegmentedControl component. */
struct SegmentedControlProps {
    /** Reconciliation key. */
    std::string key;

    /** Labels of the segments, left to right (default none). */
    std::optional<std::vector<std::string>> options;

    /** Index of the selected segment; unset selects none. */
    std::optional<std::size_t> selected;

    /** Invoked with the index of a segment when it is clicked. */
    std::function<void(std::size_t)> onChange;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a row of equally wide segments, one of which is selected.
 *
 * @param props Desired segmented control configuration for this render.
 */
void SegmentedControl(SegmentedControlProps const& props);

} /* namespace Storybook */
