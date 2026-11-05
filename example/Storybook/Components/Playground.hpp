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

/** Props for the Playground component. */
struct PlaygroundProps {
    /** Reconciliation key. */
    std::string key;

    /** Renders the showcased component, centered in the preview area. */
    std::function<void()> preview;

    /** Renders the controls, stacked in the panel on the right. */
    std::function<void()> controls;

    /** Code for the current state of the controls, shown under the preview; unset shows none. */
    std::optional<std::string> code;

    /** Props forwarded to the preview area Node; any field set here overrides the computed style. */
    Rocket::NodeProps previewProps;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a framed playground: the preview on the left, the controls on the
 * right and the code under both.
 *
 * @param props Desired playground configuration for this render.
 */
void Playground(PlaygroundProps const& props);

} /* namespace Storybook */
