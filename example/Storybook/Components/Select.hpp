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

/** Props for the Select component. */
struct SelectProps {
    /** Reconciliation key. */
    std::string key;

    /** Labels of the options, top to bottom (default none). */
    std::optional<std::vector<std::string>> options;

    /** Index of the selected option; unset leaves the field empty. */
    std::optional<std::size_t> selected;

    /** Invoked with the index of an option when it is picked. */
    std::function<void(std::size_t)> onChange;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a field showing the selected option; a click opens a dropdown
 * list of the options under it, which closes when an option is picked or a
 * press lands outside the field.
 *
 * @param props Desired select configuration for this render.
 */
void Select(SelectProps const& props);

} /* namespace Storybook */
