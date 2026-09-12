/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/Base/Profile.hpp>

namespace Rocket {

struct Mat3;
union Vec4;

/**
 * Two-component floating-point vector.
 *
 * The components are stored contiguously in data[2] and aliased as both
 * (x, y) for positional use and (width, height) for size use.
 */
union Vec2 {
    float data[2];
    struct {
        float x;
        float y;
    };
    struct {
        float width;
        float height;
    };
    constexpr Vec2()
        : x(0.0f)
        , y(0.0f) {};
    /**
     * A vector from x and y components.
     *
     * @param x The x component.
     * @param y The y component.
     */
    constexpr Vec2(float x, float y)
        : x(x)
        , y(y) {};
    Vec2 operator+(Vec2 const&) const;
    Vec2& operator+=(Vec2 const&);
    Vec2 operator-(Vec2 const&) const;
    Vec2& operator-=(Vec2 const&);
    Vec2 operator*(Vec2 const&) const;
    Vec2& operator*=(Vec2 const&);
    Vec2 operator/(Vec2 const&) const;
    Vec2& operator/=(Vec2 const&);
    Vec2 operator+(float) const;
    Vec2& operator+=(float);
    Vec2 operator-(float) const;
    Vec2& operator-=(float);
    Vec2 operator*(float) const;
    Vec2& operator*=(float);
    Vec2 operator/(float) const;
    Vec2& operator/=(float);
    bool operator==(Vec2 const&) const;
    bool operator!=(Vec2 const&) const;

    /**
     * Returns true if this point lies inside the given rectangle.
     *
     * @param rect The rectangle to test against.
     */
    bool inRect(Vec4 const& rect) const;

    /**
     * Returns true if this point lies inside the triangle formed by three vertices.
     *
     * @param a First vertex.
     * @param b Second vertex.
     * @param c Third vertex.
     */
    bool inTriangle(Vec2 const& a, Vec2 const& b, Vec2 const& c) const;

    /**
     * Returns the angle in degrees from a center point to this point, normalized to [0, 360).
     *
     * @param center The reference center point.
     */
    float getAngle(Vec2 const& center) const;

    /**
     * Returns the Euclidean distance to another point.
     *
     * @param to The target point.
     */
    float getDistance(Vec2 const& to) const;

    /**
     * Returns a point linearly interpolated toward another by t.
     *
     * @param to The target point.
     * @param t  Interpolation factor in [0, 1].
     */
    Vec2 toLerped(Vec2 const& to, float t) const;

    /**
     * Returns this point rotated around a center by an angle in degrees.
     *
     * @param center The pivot point.
     * @param angle  Rotation angle in degrees.
     */
    Vec2 toRotated(Vec2 const& center, float angle) const;

    /**
     * Returns this point offset by a delta.
     *
     * @param delta The translation to apply.
     */
    Vec2 toTranslated(Vec2 const& delta) const;

    /**
     * Returns this point transformed by a 3×3 matrix.
     *
     * @param mat The transformation matrix.
     */
    Vec2 toTransformed(Mat3 const& mat) const;
};

} /* namespace Rocket */
