/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <algorithm>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Math/Mat3.hpp>

namespace Rocket {

bool Vec4::operator==(Vec4 const& other) const {
    PROFILE

    return (x == other.x)
        && (y == other.y)
        && (width == other.width)
        && (height == other.height);
}

bool Vec4::operator!=(Vec4 const& other) const {
    PROFILE

    return !operator==(other);
}

Vec4 Vec4::operator*(float value) const {
    PROFILE

    return {
        (origin * value),
        (size * value)
    };
}

Vec4& Vec4::operator*=(float value) {
    PROFILE

    origin *= value;
    size *= value;

    return *this;
}

Vec4 Vec4::operator/(float value) const {
    PROFILE

    return {
        (origin / value),
        (size / value)
    };
}

Vec4& Vec4::operator/=(float value) {
    PROFILE

    origin /= value;
    size /= value;

    return *this;
}

float Vec4::getMaxX() const {
    PROFILE

    return (x + width);
}

float Vec4::getMaxY() const {
    PROFILE

    return (y + height);
}

Vec2 Vec4::getCenter() const {
    PROFILE

    return {
        (x + (width / 2.0f)),
        (y + (height / 2.0f))
    };
}

Vec4 Vec4::getIntersection(Vec4 const& other) const {
    PROFILE

    auto left = std::max(x, other.x);
    auto right = std::min(x + width, other.x + other.width);
    auto top = std::max(y, other.y);
    auto bottom = std::min(y + height, other.y + other.height);

    if (left >= right || top >= bottom) {
        return { 0, 0, 0, 0 };
    } else {
        return { left, top, (right - left), (bottom - top) };
    }
}

Vec2 Vec4::getEdgePoint(Vec2 const& point) const {
    PROFILE

    auto rcx = (x + (width / 2.0f));
	auto rcy = (y + (height / 2.0f));
	auto A = std::atan2(std::abs(rcy - point.y), std::abs(point.x - rcx));
	auto R_v = (height / 2.0f);
	auto R_h = (width / 2.0f);
	auto L_cot = (R_v * (1.0f / std::tan(A)));
	auto L_tan = (R_h * std::tan(A));
	auto L = L_cot;
	auto ex = 0.0f;
    auto ey = 0.0f;

	if (L_cot > R_h) {
		ex = ((rcx <= point.x) ? (x + width) : x);
		ey = ((rcy >= point.y) ? (rcy - L_tan) : (rcy + L_tan));
	} else {
		ex = ((rcx <= point.x) ? (rcx + L) : (rcx - L));
		ey = ((rcy >= point.y) ? (rcy - R_v) : (rcy + R_v));
	}

	return { ex, ey };
}

Vec4 Vec4::toScaled(Vec2 const& scale) const {
    PROFILE

    return {
        (origin * scale),
        (size * scale)
    };
}

Vec4 Vec4::toScaled(float const& scale) const {
    PROFILE

    return {
        (origin * scale),
        (size * scale)
    };
}

Vec4 Vec4::toTranslated(Vec2 const& translate) const {
    PROFILE

    return {
        (origin + translate),
        size
    };
}

Vec4 Vec4::toTransformed(Mat3 const& matrix) const {
    PROFILE

    auto const p0 = Vec2{x,         y          }.toTransformed(matrix);
    auto const p1 = Vec2{x + width, y          }.toTransformed(matrix);
    auto const p2 = Vec2{x,         y + height }.toTransformed(matrix);
    auto const p3 = Vec2{x + width, y + height }.toTransformed(matrix);

    auto minX = std::min({ p0.x, p1.x, p2.x, p3.x });
    auto minY = std::min({ p0.y, p1.y, p2.y, p3.y });
    auto maxX = std::max({ p0.x, p1.x, p2.x, p3.x });
    auto maxY = std::max({ p0.y, p1.y, p2.y, p3.y });

    return {
        minX,
        minY,
        (maxX - minX),
        (maxY - minY)
    };
}

} /* namespace Rocket */
