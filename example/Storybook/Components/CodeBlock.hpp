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

/** Props for the CodeBlock component. */
struct CodeBlockProps {
    /** Reconciliation key. */
    std::string key;

    /** The code; lines are separated by '\n' (default empty). */
    std::optional<std::string> content;

    /** Props forwarded to the text Node. */
    Rocket::NodeProps textProps;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a block of code in a monospace font on a light gray panel.
 *
 * @param props Desired code block configuration for this render.
 */
void CodeBlock(CodeBlockProps const& props);

} /* namespace Storybook */
