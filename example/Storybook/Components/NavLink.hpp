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

namespace Storybook {

/** Props for the NavLink component. */
struct NavLinkProps {
    /** Reconciliation key. */
    std::string key;

    /** Text of the link (default empty). */
    std::optional<std::string> label;

    /** Marks the link of the page that is shown (default false). */
    std::optional<bool> active;

    /** Invoked on click. */
    std::function<void()> onClick;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a navigation link for a sidebar, highlighted when active or hovered.
 *
 * @param props Desired link configuration for this render.
 */
void NavLink(NavLinkProps const& props);

} /* namespace Storybook */
