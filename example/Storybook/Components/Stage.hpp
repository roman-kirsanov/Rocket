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
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/UI/Node.hpp>

namespace Storybook {

/** Colors of a stage, the panel components are shown on: dark for a dark theme, light for a light one. */
struct StageColors {
    /** Fill of the stage. */
    Rocket::Vec4 background;
    /** Edge of the stage. */
    Rocket::Vec4 border;
    /** Color of captions shown on the stage. */
    Rocket::Vec4 text;
};

/** Returns the colors of a dark stage. */
StageColors GetDarkStageColors();

/** Returns the colors of a light stage. */
StageColors GetLightStageColors();

/** Returns the stage colors provided as a context, by App for the theme picked in the sidebar, or the dark ones when there are none. */
StageColors UseStageColors();

/** Props for the Stage component. */
struct StageProps {
    /** Reconciliation key. */
    std::string key;

    /** Colors of the stage (default those of UseStageColors). */
    std::optional<StageColors> colors;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a padded, rounded panel that shows components in a row that wraps.
 *
 * @param props    Desired stage configuration for this render.
 * @param children The components shown; default no-op.
 */
void Stage(StageProps const& props, std::function<void()> const& children = []{});

} /* namespace Storybook */
