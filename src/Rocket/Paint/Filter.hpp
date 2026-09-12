/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <Rocket/Base/Enum.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/** A post-processing filter that applies a gaussian blur to the painted shape. */
struct BlurFilter {
    /** Blur radius in pixels; must be greater than 0 to have an effect. */
    float radius;
    bool operator==(BlurFilter const&) const;
    bool operator!=(BlurFilter const&) const;
};

/** A post-processing filter that renders a blurred, tinted shadow of the painted shape's silhouette (drop or inset) instead of the shape itself. */
struct ShadowFilter {
    /** Blur radius in pixels; 0 produces a hard-edged shadow. */
    float radius;
    /** Shadow RGBA color (non-premultiplied). */
    Vec4 color;
    /** Displacement of the shadow in pixels. */
    std::optional<Vec2> offset;
    /** Amount in pixels by which the shadow field is dilated (for inset shadows this thickens the shadow inward). */
    std::optional<Vec2> spread;
    /** Casts the shadow inward from the shape's edges, clipped to the silhouette, instead of as a drop shadow. */
    bool inset;
    bool operator==(ShadowFilter const&) const;
    bool operator!=(ShadowFilter const&) const;
};

/** A post-processing filter applied to a painted shape. */
using Filter = Enum<BlurFilter, ShadowFilter>;

} /* namespace Rocket */
