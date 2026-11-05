/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <functional>
#include <string>
#include <Rocket/UI/Node.hpp>

namespace Storybook {

/** Props for the ColorSchemeToggle component. */
struct ColorSchemeToggleProps {
    /** Reconciliation key. */
    std::string key;

    /** Invoked on click. */
    std::function<void()> onClick;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a square icon button that switches the color scheme: a moon in the
 * light scheme and a sun in the dark one.
 *
 * @param props Desired toggle configuration for this render.
 */
void ColorSchemeToggle(ColorSchemeToggleProps const& props);

} /* namespace Storybook */
