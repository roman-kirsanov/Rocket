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

Vec2 Vec2::operator+(Vec2 const& other) const {
    PROFILE

    return Vec2{x + other.x, y + other.y};
}

Vec2& Vec2::operator+=(Vec2 const& other) {
    PROFILE

    x += other.x;
    y += other.y;

    return *this;
}

Vec2 Vec2::operator-(Vec2 const& other) const {
    PROFILE

    return Vec2{x - other.x, y - other.y};
}

Vec2& Vec2::operator-=(Vec2 const& other) {
    PROFILE

    x -= other.x;
    y -= other.y;

    return *this;
}

Vec2 Vec2::operator*(Vec2 const& other) const {
    PROFILE

    return Vec2{x * other.x, y * other.y};
}

Vec2& Vec2::operator*=(Vec2 const& other) {
    PROFILE

    x *= other.x;
    y *= other.y;

    return *this;
}

Vec2 Vec2::operator/(Vec2 const& other) const {
    PROFILE

    return Vec2{x / other.x, y / other.y};
}

Vec2& Vec2::operator/=(Vec2 const& other) {
    PROFILE

    x /= other.x;
    y /= other.y;

    return *this;
}

Vec2 Vec2::operator+(float scalar) const {
    PROFILE

    return Vec2{x + scalar, y + scalar};
}

Vec2& Vec2::operator+=(float scalar) {
    PROFILE

    x += scalar;
    y += scalar;

    return *this;
}

Vec2 Vec2::operator-(float scalar) const {
    PROFILE

    return Vec2{x - scalar, y - scalar};
}

Vec2& Vec2::operator-=(float scalar) {
    PROFILE

    x -= scalar;
    y -= scalar;

    return *this;
}

Vec2 Vec2::operator*(float scalar) const {
    PROFILE

    return {
        x * scalar,
        y * scalar
    };
}

Vec2& Vec2::operator*=(float scalar) {
    PROFILE

    x *= scalar;
    y *= scalar;

    return *this;
}

Vec2 Vec2::operator/(float scalar) const {
    PROFILE

    return {
        x / scalar,
        y / scalar
    };
}

Vec2& Vec2::operator/=(float scalar) {
    PROFILE

    x /= scalar;
    y /= scalar;

    return *this;
}

bool Vec2::operator==(Vec2 const& other) const {
    PROFILE

    return (x == other.x)
        && (y == other.y);
}

bool Vec2::operator!=(Vec2 const& other) const {
    PROFILE

    return !operator==(other);
}

bool Vec2::inRect(Vec4 const& rect) const {
    PROFILE

    return (x >= rect.x)
        && (y >= rect.y)
        && (x < rect.getMaxX())
        && (y < rect.getMaxY());
}

bool Vec2::inTriangle(Vec2 const& point1, Vec2 const& point2, Vec2 const& point3) const {
    PROFILE

    static auto sign = [](Vec2 const& point1, Vec2 const& point2, Vec2 const& point3) {
            return (point1.x - point3.x) * (point2.y - point3.y) - (point2.x - point3.x) * (point1.y - point3.y);
    };

    auto d1 = sign(*this, point1, point2);
    auto d2 = sign(*this, point2, point3);
    auto d3 = sign(*this, point3, point1);
    auto neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    auto pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(neg && pos);
}

float Vec2::getAngle(Vec2 const& center) const {
    PROFILE

    auto const dx = (x - center.x);
    auto const dy = (y - center.y);
    auto const an = ((std::atan2(dy, dx) * 180.0f) / PI);

    return (an < 0 ? an + 360.0f : an);
}

float Vec2::getDistance(Vec2 const& point) const {
    PROFILE

    return std::abs(std::sqrtf(((x - point.x) * (x - point.x)) + ((y - point.y) * (y - point.y))));
}

Vec2 Vec2::toLerped(Vec2 const& stop, float t) const {
    PROFILE

    return {
        (x + (stop.x - x) * t),
        (y + (stop.y - y) * t)
    };
}

Vec2 Vec2::toRotated(Vec2 const& center, float angle) const {
    PROFILE

    auto const rad = -((PI / 180.0f) * angle);
    auto const co = std::cos(rad);
    auto const si = std::sin(rad);

    return {
        ((co * (x - center.x)) + (si * (y - center.y)) + center.x),
        ((co * (y - center.y)) - (si * (x - center.x)) + center.y)
    };
}

Vec2 Vec2::toTranslated(Vec2 const& point) const {
    PROFILE

    return (*this + point);
}

Vec2 Vec2::toTransformed(Mat3 const& matrix) const {
    PROFILE

    return {
        x * matrix.data[0] + y * matrix.data[3] + matrix.data[6],
        x * matrix.data[1] + y * matrix.data[4] + matrix.data[7]
    };
}

} /* namespace Rocket */
