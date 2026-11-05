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

/** Heading level of a title. */
enum class TitleOrder {
    H1,
    H2,
    H3
};

/** Props for the Title component. */
struct TitleProps {
    /** Reconciliation key. */
    std::string key;

    /** Text of the title (default empty). */
    std::optional<std::string> content;

    /** Heading level; sets the font size (default H1). */
    std::optional<TitleOrder> order;

    /** Props forwarded to the text Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a bold heading.
 *
 * @param props Desired title configuration for this render.
 */
void Title(TitleProps const& props);

} /* namespace Storybook */
