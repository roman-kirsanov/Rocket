/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/**
 * Drop shadow descriptor.
 *
 * All fields are optional; unset fields fall back to renderer defaults
 * (except inset, which is not yet honored).
 */
struct Shadow {
    /** Shadow RGBA color. */
    std::optional<Vec4> color;
    /** Horizontal and vertical displacement of the shadow. */
    std::optional<Vec2> offset;
    /** Amount by which the shadow is expanded beyond the shape boundary. */
    std::optional<Vec2> spread;
    /** Blur radius in pixels. */
    std::optional<float> blur;
    /** Casts the shadow inward from the shape's edges instead of as a drop shadow. Not yet honored by the renderer. */
    std::optional<bool> inset;
    bool operator==(Shadow const&) const;
    bool operator!=(Shadow const&) const;
};

} /* namespace Rocket */
