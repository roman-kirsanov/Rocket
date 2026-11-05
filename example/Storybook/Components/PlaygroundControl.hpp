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

/** Props for the PlaygroundControl component. */
struct PlaygroundControlProps {
    /** Reconciliation key. */
    std::string key;

    /** Name of the prop the control sets, shown above it (default empty). */
    std::optional<std::string> label;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders one control of a playground under the name of the prop it sets.
 *
 * @param props    Desired control configuration for this render.
 * @param children The control itself; default no-op.
 */
void PlaygroundControl(PlaygroundControlProps const& props, std::function<void()> const& children = []{});

} /* namespace Storybook */
