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
    /** Blur radius in pixels, as CSS filter: blur() defines it: the gaussian's standard deviation. Must be greater than 0 to have an effect. */
    float radius;
    /** Standard deviation of the gaussian in pixels. */
    float getSigma() const;
    /** Distance in pixels the blur reaches from a pixel (the kernel's 3-sigma tail). */
    float getExtent() const;
    bool operator==(BlurFilter const&) const;
    bool operator!=(BlurFilter const&) const;
};

/** A post-processing filter that renders a blurred, tinted shadow of the painted shape's silhouette (drop or inset) instead of the shape itself. */
struct ShadowFilter {
    /** Blur radius in pixels, as CSS box-shadow defines it: twice the gaussian's standard deviation. 0 produces a hard-edged shadow. */
    float radius;
    /** Shadow RGBA color (non-premultiplied). */
    Vec4 color;
    /** Displacement of the shadow in pixels. */
    std::optional<Vec2> offset;
    /** Amount in pixels by which the shadow field is dilated before blurring (for inset shadows this thickens the shadow inward); negative values erode it instead. */
    std::optional<Vec2> spread;
    /** Casts the shadow inward from the shape's edges, clipped to the silhouette, instead of as a drop shadow. */
    bool inset;
    /** Standard deviation of the gaussian in pixels. */
    float getSigma() const;
    /** Distance in pixels the blurred shadow reaches beyond the (spread) silhouette (the kernel's 3-sigma tail). */
    float getExtent() const;
    bool operator==(ShadowFilter const&) const;
    bool operator!=(ShadowFilter const&) const;
};

/** A post-processing filter applied to a painted shape. */
using Filter = Enum<BlurFilter, ShadowFilter>;

} /* namespace Rocket */
