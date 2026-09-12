/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Math/Float.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Math/Mat3.hpp>

namespace Rocket {

bool Mat3::operator==(Mat3 const& other) const {
    PROFILE

    for (int i = 0; i < 9; i++) {
        if (data[i] != other.data[i]) {
            return false;
        }
    }

    return true;
}

bool Mat3::operator!=(Mat3 const& other) const {
    PROFILE

    return !operator==(other);
}

Mat3& Mat3::operator*=(Mat3 const& other) {
    PROFILE

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            data[i * 3 + j] = (
                data[i * 3 + 0] * other.data[0 * 3 + j] +
                data[i * 3 + 1] * other.data[1 * 3 + j] +
                data[i * 3 + 2] * other.data[2 * 3 + j]
            );
        }
    }

    return *this;
}

Mat3 Mat3::operator*(Mat3 const& other) const {
    PROFILE

    auto result = Mat3{};

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.data[i * 3 + j] = (
                data[i * 3 + 0] * other.data[0 * 3 + j] +
                data[i * 3 + 1] * other.data[1 * 3 + j] +
                data[i * 3 + 2] * other.data[2 * 3 + j]
            );
        }
    }

    return result;
}

Mat3 Mat3::toRotated(float angle) const {
    PROFILE

    auto rad = (angle * (PI / 180.0f));
    auto cos = std::cosf(rad);
    auto sin = std::sinf(rad);

    return operator*({
        cos,  sin,  0.0f,
        -sin, cos,  0.0f,
        0.0f, 0.0f, 1.0f
    });
}

Mat3 Mat3::toScaled(Vec2 const& point) const {
    PROFILE

    return operator*({
        point.x, 0.0f,    0.0f,
        0.0f,    point.y, 0.0f,
        0.0f,    0.0f,    1.0f
    });
}

Mat3 Mat3::toTranslated(Vec2 const& point) const {
    PROFILE

    return operator*({
        1.0f,    0.0f,    0.0f,
        0.0f,    1.0f,    0.0f,
        point.x, point.y, 1.0f
    });
}

Mat3 Mat3::OrthoTopLeft(Vec4 const& rect) {
    PROFILE

    auto l = rect.x;
    auto t = rect.y;
    auto r = rect.getMaxX();
    auto b = rect.getMaxY();

    return Mat3{
        2.0f / (r - l), 0.0f, 0.0f,
        0.0f, -2.0f / (b - t), 0.0f,
        -(r + l) / (r - l), (b + t) / (b - t), 1.0f
    };
}

Mat3 Mat3::OrthoBottomLeft(Vec4 const& rect) {
    PROFILE

    auto l = rect.x;
    auto b = rect.y;
    auto r = rect.getMaxX();
    auto t = rect.getMaxY();

    return Mat3{
        2.0f / (r - l), 0.0f, 0.0f,
        0.0f, 2.0f / (t - b), 0.0f,
        -(r + l) / (r - l), -(t + b) / (t - b), 1.0f
    };
}

} /* namespace Rocket */
