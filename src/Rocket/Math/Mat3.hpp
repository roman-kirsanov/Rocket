/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/**
 * 3×3 row-major affine transformation matrix.
 *
 * Elements are stored in data[9] in row-major order.
 * The default constructor initializes to the identity matrix.
 */
struct Mat3 {
    float data[9];
    constexpr Mat3()
        : data{1.0f, 0.0f, 0.0f,
               0.0f, 1.0f, 0.0f,
               0.0f, 0.0f, 1.0f} {}
    /**
     * A matrix from its nine elements in row-major order.
     *
     * @param a00 Row 0, column 0.
     * @param a01 Row 0, column 1.
     * @param a02 Row 0, column 2.
     * @param a10 Row 1, column 0.
     * @param a11 Row 1, column 1.
     * @param a12 Row 1, column 2.
     * @param a20 Row 2, column 0.
     * @param a21 Row 2, column 1.
     * @param a22 Row 2, column 2.
     */
    constexpr Mat3(float a00, float a01, float a02,
                   float a10, float a11, float a12,
                   float a20, float a21, float a22)
        : data{a00, a01, a02,
               a10, a11, a12,
               a20, a21, a22} {}
    bool operator==(Mat3 const&) const;
    bool operator!=(Mat3 const&) const;
    Mat3& operator*=(Mat3 const&);
    Mat3 operator*(Mat3 const&) const;

    /**
     * Returns this matrix with a rotation appended.
     *
     * @param angle Rotation angle in degrees.
     */
    Mat3 toRotated(float angle) const;

    /**
     * Returns this matrix with a scale appended.
     *
     * @param scale Scale factors for x and y axes.
     */
    Mat3 toScaled(Vec2 const& scale) const;

    /**
     * Returns this matrix with a translation appended.
     *
     * @param offset The translation to apply.
     */
    Mat3 toTranslated(Vec2 const& offset) const;

    /**
     * Builds an orthographic projection for the given viewport with the origin at top-left (Y down).
     *
     * @param viewport The target viewport rectangle.
     */
    static Mat3 OrthoTopLeft(Vec4 const& viewport);

    /**
     * Builds an orthographic projection for the given viewport with the origin at bottom-left (Y up).
     *
     * @param viewport The target viewport rectangle.
     */
    static Mat3 OrthoBottomLeft(Vec4 const& viewport);
};

} /* namespace Rocket */
