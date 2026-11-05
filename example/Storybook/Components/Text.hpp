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

/** Font size of a text. */
enum class TextSize {
    Medium,
    Small
};

/** Props for the Text component. */
struct TextProps {
    /** Reconciliation key. */
    std::string key;

    /** The text (default empty). */
    std::optional<std::string> content;

    /** Font size (default Medium). */
    std::optional<TextSize> size;

    /** Draws the text in a lighter gray, for secondary text (default false). */
    std::optional<bool> dimmed;

    /** Props forwarded to the text Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders body text that wraps to the width of its parent.
 *
 * @param props Desired text configuration for this render.
 */
void Text(TextProps const& props);

} /* namespace Storybook */
