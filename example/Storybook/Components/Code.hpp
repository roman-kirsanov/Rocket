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

namespace Storybook {

/** Props for the Code component. */
struct CodeProps {
    /** Reconciliation key. */
    std::string key;

    /** The code (default empty). */
    std::optional<std::string> content;

    /** Props forwarded to the text Node. */
    Rocket::NodeProps textProps;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a short piece of code in a monospace font on a gray chip.
 *
 * The font is the family "Mono", which the app registers at start-up; the
 * default font is used when it is not registered.
 *
 * @param props Desired code configuration for this render.
 */
void Code(CodeProps const& props);

} /* namespace Storybook */
