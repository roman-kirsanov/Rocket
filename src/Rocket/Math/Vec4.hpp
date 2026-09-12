/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Math/Vec2.hpp>

namespace Rocket {

struct Mat3;

/**
 * Four-component floating-point vector with three named interpretations.
 *
 * The components in data[4] are aliased as:
 *   - a rectangle: (x, y, width, height) with an origin/size pair
 *   - edge insets:  (left, top, right, bottom)
 *   - a color:      (red, green, blue, alpha)
 */
union Vec4 {
    float data[4];
    struct {
        float x;
        float y;
        float width;
        float height;
    };
    struct {
        float left;
        float top;
        float right;
        float bottom;
    };
    struct {
        float red;
        float green;
        float blue;
        float alpha;
    };
    struct {
        Vec2 origin;
        Vec2 size;
    };
    constexpr Vec4()
        : x(0.0f)
        , y(0.0f)
        , width(0.0f)
        , height(0.0f) {};
    /**
     * A rectangle from an origin and a size.
     *
     * @param origin The rectangle's origin point.
     * @param size   The rectangle's size.
     */
    constexpr Vec4(Vec2 origin, Vec2 size)
        : origin(origin)
        , size(size) {};

    /**
     * A vector from four components (rectangle x, y, width, height).
     *
     * @param x      The x coordinate.
     * @param y      The y coordinate.
     * @param width  The width.
     * @param height The height.
     */
    constexpr Vec4(float x, float y, float width, float height)
        : x(x)
        , y(y)
        , width(width)
        , height(height) {};
    bool operator==(Vec4 const&) const;
    bool operator!=(Vec4 const&) const;
    Vec4 operator*(float) const;
    Vec4& operator*=(float);
    Vec4 operator/(float) const;
    Vec4& operator/=(float);

    /** Returns x + width (the right edge of the rectangle). */
    float getMaxX() const;

    /** Returns y + height (the bottom edge of the rectangle). */
    float getMaxY() const;

    /** Returns the midpoint of the rectangle. */
    Vec2 getCenter() const;

    /**
     * Returns the overlapping region with another rectangle.
     *
     * @param other The rectangle to intersect with.
     */
    Vec4 getIntersection(Vec4 const& other) const;

    /**
     * Returns the point where a ray from the rectangle's center through the given point crosses the rectangle's boundary.
     *
     * @param point A point whose direction from the rectangle's center determines the edge point.
     */
    Vec2 getEdgePoint(Vec2 const& point) const;

    /**
     * Returns this rectangle offset by a delta.
     *
     * @param delta The translation to apply.
     */
    Vec4 toTranslated(Vec2 const& delta) const;

    /**
     * Returns this rectangle scaled by per-axis factors.
     *
     * @param scale Scale factors for x and y axes.
     */
    Vec4 toScaled(Vec2 const& scale) const;

    /**
     * Returns this rectangle scaled uniformly.
     *
     * @param scale Uniform scale factor.
     */
    Vec4 toScaled(float const& scale) const;

    /**
     * Returns this rectangle transformed by a 3×3 matrix.
     *
     * @param mat The transformation matrix.
     */
    Vec4 toTransformed(Mat3 const& mat) const;
};

} /* namespace Rocket */
