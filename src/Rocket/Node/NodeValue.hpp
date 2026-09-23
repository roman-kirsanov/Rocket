/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/Base/Enum.hpp>

namespace Rocket {

/** A layout length expressed in document points (scaled by the document scale when rendered). */
struct PixelValue {
    float value;
    bool operator==(PixelValue const&) const;
    bool operator!=(PixelValue const&) const;
};

/** A layout length expressed as a percentage: of the parent dimension for
    layout properties (gaps resolve against the node's own inner size), or of
    the node's own size for NodeTransform translation. */
struct PercentValue {
    float value;
    bool operator==(PercentValue const&) const;
    bool operator!=(PercentValue const&) const;
};

/**
 * A layout length that is either an absolute pixel value or a percentage.
 *
 * Wraps Enum<PixelValue, PercentValue>. A bare float literal constructs a
 * PixelValue; use PercentValue{n} to construct a percentage. Used for the
 * sizing and spacing layout properties on Node (width, height, min/max sizes,
 * padding, margin, gap, position edges) and for NodeTransform translation;
 * border widths, radii, and font size are plain floats.
 */
struct NodeValue : Enum<PixelValue, PercentValue> {
    using Enum<PixelValue, PercentValue>::Enum;
    /**
     * Constructs a pixel value from a bare float.
     *
     * @param value The pixel value.
     */
    constexpr NodeValue(float value) noexcept
        : Enum<PixelValue, PercentValue>(PixelValue{value}) {}
};

} /* namespace Rocket */
