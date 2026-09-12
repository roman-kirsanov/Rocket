/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Mat3.hpp>
#include <cmath>
#include <gtest/gtest.h>

using namespace Rocket;

/* The default constructor zero-initializes all four components. */
TEST(Vec4, DefaultConstructorZero) {
    Vec4 v;
    ASSERT_TRUE(v.x == 0.0f);
    ASSERT_TRUE(v.y == 0.0f);
    ASSERT_TRUE(v.width == 0.0f);
    ASSERT_TRUE(v.height == 0.0f);
}

/* The four-float constructor stores components in declaration order. */
TEST(Vec4, FourFloatConstructor) {
    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_TRUE(v.x == 1.0f);
    ASSERT_TRUE(v.y == 2.0f);
    ASSERT_TRUE(v.width == 3.0f);
    ASSERT_TRUE(v.height == 4.0f);
    ASSERT_TRUE(v.data[0] == 1.0f);
    ASSERT_TRUE(v.data[1] == 2.0f);
    ASSERT_TRUE(v.data[2] == 3.0f);
    ASSERT_TRUE(v.data[3] == 4.0f);
}

/* The (origin, size) constructor fills the rect interpretation. */
TEST(Vec4, OriginSizeConstructor) {
    Vec4 v(Vec2(1.0f, 2.0f), Vec2(3.0f, 4.0f));
    ASSERT_TRUE(v == Vec4(1.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_TRUE(v.origin == Vec2(1.0f, 2.0f));
    ASSERT_TRUE(v.size == Vec2(3.0f, 4.0f));
}

/* The inset aliases (left, top, right, bottom) share storage with x/y/width/height. */
TEST(Vec4, InsetAliases) {
    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_TRUE(v.left == 1.0f);
    ASSERT_TRUE(v.top == 2.0f);
    ASSERT_TRUE(v.right == 3.0f);
    ASSERT_TRUE(v.bottom == 4.0f);
    v.right = 30.0f;
    ASSERT_TRUE(v.width == 30.0f);
}

/* The color aliases (red, green, blue, alpha) share storage with x/y/width/height. */
TEST(Vec4, ColorAliases) {
    Vec4 v(0.25f, 0.5f, 0.75f, 1.0f);
    ASSERT_TRUE(v.red == 0.25f);
    ASSERT_TRUE(v.green == 0.5f);
    ASSERT_TRUE(v.blue == 0.75f);
    ASSERT_TRUE(v.alpha == 1.0f);
    v.red = 0.125f;
    ASSERT_TRUE(v.x == 0.125f);
    ASSERT_TRUE(v.data[0] == 0.125f);
}

/* Equality compares all four components; inequality is its negation. */
TEST(Vec4, EqualityInequality) {
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) == Vec4(1.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_TRUE(!(Vec4(1.0f, 2.0f, 3.0f, 4.0f) != Vec4(1.0f, 2.0f, 3.0f, 4.0f)));
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) != Vec4(0.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) != Vec4(1.0f, 0.0f, 3.0f, 4.0f));
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) != Vec4(1.0f, 2.0f, 0.0f, 4.0f));
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) != Vec4(1.0f, 2.0f, 3.0f, 0.0f));
}

/* Scalar multiplication and division scale all four components. */
TEST(Vec4, ScalarMultiplicationDivision) {
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f == Vec4(2.0f, 4.0f, 6.0f, 8.0f));
    ASSERT_TRUE(Vec4(2.0f, 4.0f, 6.0f, 8.0f) / 2.0f == Vec4(1.0f, 2.0f, 3.0f, 4.0f));

    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    v *= 2.0f;
    ASSERT_TRUE(v == Vec4(2.0f, 4.0f, 6.0f, 8.0f));
    v /= 4.0f;
    ASSERT_TRUE(v == Vec4(0.5f, 1.0f, 1.5f, 2.0f));
}

/* getMaxX and getMaxY return the far edges of the rectangle. */
TEST(Vec4, GetMaxXMaxY) {
    Vec4 rect(10.0f, 20.0f, 30.0f, 40.0f);
    ASSERT_TRUE(rect.getMaxX() == 40.0f);
    ASSERT_TRUE(rect.getMaxY() == 60.0f);
}

/* getCenter returns the rectangle's midpoint. */
TEST(Vec4, GetCenter) {
    ASSERT_TRUE(Vec4(10.0f, 20.0f, 30.0f, 40.0f).getCenter() == Vec2(25.0f, 40.0f));
    ASSERT_TRUE(Vec4().getCenter() == Vec2(0.0f, 0.0f));
}

/* getIntersection returns the overlapping region of two rectangles. */
TEST(Vec4, GetIntersectionOverlap) {
    Vec4 a(0.0f, 0.0f, 10.0f, 10.0f);
    Vec4 b(5.0f, 5.0f, 10.0f, 10.0f);
    ASSERT_TRUE(a.getIntersection(b) == Vec4(5.0f, 5.0f, 5.0f, 5.0f));
    ASSERT_TRUE(b.getIntersection(a) == Vec4(5.0f, 5.0f, 5.0f, 5.0f));
    ASSERT_TRUE(a.getIntersection(Vec4(2.0f, 3.0f, 4.0f, 5.0f)) == Vec4(2.0f, 3.0f, 4.0f, 5.0f));
}

/* getIntersection of disjoint or merely touching rectangles is the zero rect. */
TEST(Vec4, GetIntersectionDisjoint) {
    Vec4 a(0.0f, 0.0f, 10.0f, 10.0f);
    ASSERT_TRUE(a.getIntersection(Vec4(20.0f, 20.0f, 5.0f, 5.0f)) == Vec4());
    ASSERT_TRUE(a.getIntersection(Vec4(10.0f, 0.0f, 5.0f, 5.0f)) == Vec4());
    ASSERT_TRUE(a.getIntersection(Vec4(0.0f, 10.0f, 5.0f, 5.0f)) == Vec4());
}

/* getEdgePoint returns the boundary point toward the given point. */
TEST(Vec4, GetEdgePoint) {
    Vec4 rect(0.0f, 0.0f, 10.0f, 10.0f);

    auto right = rect.getEdgePoint(Vec2(100.0f, 5.0f));
    ASSERT_TRUE(std::abs(right.x - 10.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(right.y - 5.0f) < 1e-5f);

    auto left = rect.getEdgePoint(Vec2(-100.0f, 5.0f));
    ASSERT_TRUE(std::abs(left.x - 0.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(left.y - 5.0f) < 1e-5f);

    auto top = rect.getEdgePoint(Vec2(5.0f, -100.0f));
    ASSERT_TRUE(std::abs(top.x - 5.0f) < 1e-4f);
    ASSERT_TRUE(std::abs(top.y - 0.0f) < 1e-5f);

    auto corner = rect.getEdgePoint(Vec2(100.0f, 100.0f));
    ASSERT_TRUE(std::abs(corner.x - 10.0f) < 1e-4f);
    ASSERT_TRUE(std::abs(corner.y - 10.0f) < 1e-4f);
}

/* toTranslated offsets the origin and keeps the size. */
TEST(Vec4, ToTranslated) {
    auto rect = Vec4(1.0f, 2.0f, 3.0f, 4.0f).toTranslated(Vec2(10.0f, 20.0f));
    ASSERT_TRUE(rect == Vec4(11.0f, 22.0f, 3.0f, 4.0f));
}

/* toScaled scales both the origin and the size, per-axis and uniformly. */
TEST(Vec4, ToScaled) {
    auto rect = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_TRUE(rect.toScaled(Vec2(2.0f, 3.0f)) == Vec4(2.0f, 6.0f, 6.0f, 12.0f));
    ASSERT_TRUE(rect.toScaled(2.0f) == Vec4(2.0f, 4.0f, 6.0f, 8.0f));
}

/* toTransformed with the identity matrix returns the rectangle unchanged. */
TEST(Vec4, ToTransformedIdentity) {
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f).toTransformed(Mat3{}) == Vec4(1.0f, 2.0f, 3.0f, 4.0f));
}

/* toTransformed with a translation matrix moves the rectangle. */
TEST(Vec4, ToTransformedTranslation) {
    Mat3 m(1.0f, 0.0f, 0.0f,
           0.0f, 1.0f, 0.0f,
           10.0f, 20.0f, 1.0f);
    ASSERT_TRUE(Vec4(1.0f, 2.0f, 3.0f, 4.0f).toTransformed(m) == Vec4(11.0f, 22.0f, 3.0f, 4.0f));
}

/* toTransformed with a rotation returns the axis-aligned bounding box. */
TEST(Vec4, ToTransformedRotation) {
    auto rect = Vec4(0.0f, 0.0f, 2.0f, 4.0f).toTransformed(Mat3{}.toRotated(90.0f));
    ASSERT_TRUE(std::abs(rect.x - (-4.0f)) < 1e-5f);
    ASSERT_TRUE(std::abs(rect.y - 0.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(rect.width - 4.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(rect.height - 2.0f) < 1e-5f);
}
