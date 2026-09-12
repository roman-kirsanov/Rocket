/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Math/Mat3.hpp>
#include <cmath>
#include <gtest/gtest.h>

using namespace Rocket;

/* The default constructor zero-initializes both components. */
TEST(Vec2, DefaultConstructorZero) {
    Vec2 v;
    ASSERT_TRUE(v.x == 0.0f);
    ASSERT_TRUE(v.y == 0.0f);
}

/* The component constructor stores x and y, aliased as width/height and data[]. */
TEST(Vec2, ComponentConstructorAliases) {
    Vec2 v(3.0f, 4.0f);
    ASSERT_TRUE(v.x == 3.0f);
    ASSERT_TRUE(v.y == 4.0f);
    ASSERT_TRUE(v.width == 3.0f);
    ASSERT_TRUE(v.height == 4.0f);
    ASSERT_TRUE(v.data[0] == 3.0f);
    ASSERT_TRUE(v.data[1] == 4.0f);
}

/* Equality compares both components; inequality is its negation. */
TEST(Vec2, EqualityInequality) {
    ASSERT_TRUE(Vec2(1.0f, 2.0f) == Vec2(1.0f, 2.0f));
    ASSERT_TRUE(!(Vec2(1.0f, 2.0f) != Vec2(1.0f, 2.0f)));
    ASSERT_TRUE(Vec2(1.0f, 2.0f) != Vec2(1.0f, 3.0f));
    ASSERT_TRUE(Vec2(1.0f, 2.0f) != Vec2(2.0f, 2.0f));
}

/* Vector arithmetic operates component-wise. */
TEST(Vec2, VectorArithmetic) {
    ASSERT_TRUE(Vec2(1.0f, 2.0f) + Vec2(3.0f, 4.0f) == Vec2(4.0f, 6.0f));
    ASSERT_TRUE(Vec2(5.0f, 7.0f) - Vec2(1.0f, 2.0f) == Vec2(4.0f, 5.0f));
    ASSERT_TRUE(Vec2(2.0f, 3.0f) * Vec2(4.0f, 5.0f) == Vec2(8.0f, 15.0f));
    ASSERT_TRUE(Vec2(8.0f, 9.0f) / Vec2(2.0f, 3.0f) == Vec2(4.0f, 3.0f));
}

/* Scalar arithmetic applies the scalar to both components. */
TEST(Vec2, ScalarArithmetic) {
    ASSERT_TRUE(Vec2(1.0f, 2.0f) + 3.0f == Vec2(4.0f, 5.0f));
    ASSERT_TRUE(Vec2(5.0f, 7.0f) - 2.0f == Vec2(3.0f, 5.0f));
    ASSERT_TRUE(Vec2(2.0f, 3.0f) * 4.0f == Vec2(8.0f, 12.0f));
    ASSERT_TRUE(Vec2(8.0f, 6.0f) / 2.0f == Vec2(4.0f, 3.0f));
}

/* Compound assignment operators modify in place and return the object. */
TEST(Vec2, CompoundAssignment) {
    Vec2 v(1.0f, 2.0f);
    v += Vec2(1.0f, 1.0f);
    ASSERT_TRUE(v == Vec2(2.0f, 3.0f));
    v -= Vec2(1.0f, 1.0f);
    ASSERT_TRUE(v == Vec2(1.0f, 2.0f));
    v *= Vec2(2.0f, 3.0f);
    ASSERT_TRUE(v == Vec2(2.0f, 6.0f));
    v /= Vec2(2.0f, 3.0f);
    ASSERT_TRUE(v == Vec2(1.0f, 2.0f));
    v += 1.0f;
    ASSERT_TRUE(v == Vec2(2.0f, 3.0f));
    v -= 1.0f;
    ASSERT_TRUE(v == Vec2(1.0f, 2.0f));
    v *= 4.0f;
    ASSERT_TRUE(v == Vec2(4.0f, 8.0f));
    v /= 2.0f;
    ASSERT_TRUE(v == Vec2(2.0f, 4.0f));
    ASSERT_TRUE((v += 1.0f) == Vec2(3.0f, 5.0f));
}

/* inRect is inclusive of the min edges and exclusive of the max edges. */
TEST(Vec2, InRect) {
    Vec4 rect(10.0f, 20.0f, 30.0f, 40.0f);
    ASSERT_TRUE(Vec2(10.0f, 20.0f).inRect(rect));
    ASSERT_TRUE(Vec2(25.0f, 40.0f).inRect(rect));
    ASSERT_TRUE(Vec2(39.0f, 59.0f).inRect(rect));
    ASSERT_TRUE(!Vec2(40.0f, 30.0f).inRect(rect));
    ASSERT_TRUE(!Vec2(25.0f, 60.0f).inRect(rect));
    ASSERT_TRUE(!Vec2(9.0f, 30.0f).inRect(rect));
    ASSERT_TRUE(!Vec2(25.0f, 19.0f).inRect(rect));
}

/* inTriangle accepts interior and edge points and rejects exterior points. */
TEST(Vec2, InTriangle) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(10.0f, 0.0f);
    Vec2 c(0.0f, 10.0f);
    ASSERT_TRUE(Vec2(2.0f, 2.0f).inTriangle(a, b, c));
    ASSERT_TRUE(Vec2(5.0f, 0.0f).inTriangle(a, b, c));
    ASSERT_TRUE(Vec2(0.0f, 0.0f).inTriangle(a, b, c));
    ASSERT_TRUE(!Vec2(6.0f, 6.0f).inTriangle(a, b, c));
    ASSERT_TRUE(!Vec2(-1.0f, 5.0f).inTriangle(a, b, c));
}

/* getAngle returns the angle in degrees from the center, normalized to [0, 360). */
TEST(Vec2, GetAngle) {
    Vec2 center(0.0f, 0.0f);
    ASSERT_TRUE(std::abs(Vec2(1.0f, 0.0f).getAngle(center) - 0.0f) < 1e-4f);
    ASSERT_TRUE(std::abs(Vec2(0.0f, 1.0f).getAngle(center) - 90.0f) < 1e-4f);
    ASSERT_TRUE(std::abs(Vec2(-1.0f, 0.0f).getAngle(center) - 180.0f) < 1e-4f);
    ASSERT_TRUE(std::abs(Vec2(0.0f, -1.0f).getAngle(center) - 270.0f) < 1e-4f);
    ASSERT_TRUE(std::abs(Vec2(6.0f, 5.0f).getAngle(Vec2(5.0f, 5.0f)) - 0.0f) < 1e-4f);
}

/* getDistance returns the Euclidean distance (3-4-5 triangle). */
TEST(Vec2, GetDistance) {
    ASSERT_TRUE(Vec2(0.0f, 0.0f).getDistance(Vec2(3.0f, 4.0f)) == 5.0f);
    ASSERT_TRUE(Vec2(3.0f, 4.0f).getDistance(Vec2(0.0f, 0.0f)) == 5.0f);
    ASSERT_TRUE(Vec2(1.0f, 2.0f).getDistance(Vec2(1.0f, 2.0f)) == 0.0f);
}

/* toLerped returns the start at t=0, the end at t=1 and the midpoint at t=0.5. */
TEST(Vec2, ToLerped) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(10.0f, 20.0f);
    ASSERT_TRUE(a.toLerped(b, 0.0f) == a);
    ASSERT_TRUE(a.toLerped(b, 1.0f) == b);
    ASSERT_TRUE(a.toLerped(b, 0.5f) == Vec2(5.0f, 10.0f));
}

/* toRotated by 90 degrees around the origin maps (1,0) to (0,1). */
TEST(Vec2, ToRotated90) {
    auto p = Vec2(1.0f, 0.0f).toRotated(Vec2(0.0f, 0.0f), 90.0f);
    ASSERT_TRUE(std::abs(p.x - 0.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(p.y - 1.0f) < 1e-5f);
}

/* toRotated by 360 degrees around any center returns the original point. */
TEST(Vec2, ToRotated360) {
    auto p = Vec2(3.0f, 4.0f).toRotated(Vec2(1.0f, 1.0f), 360.0f);
    ASSERT_TRUE(std::abs(p.x - 3.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(p.y - 4.0f) < 1e-5f);
}

/* toRotated by 180 degrees around a center mirrors the point through it. */
TEST(Vec2, ToRotated180) {
    auto p = Vec2(3.0f, 1.0f).toRotated(Vec2(1.0f, 1.0f), 180.0f);
    ASSERT_TRUE(std::abs(p.x - (-1.0f)) < 1e-5f);
    ASSERT_TRUE(std::abs(p.y - 1.0f) < 1e-5f);
}

/* toTranslated offsets the point by the delta. */
TEST(Vec2, ToTranslated) {
    ASSERT_TRUE(Vec2(1.0f, 2.0f).toTranslated(Vec2(3.0f, -5.0f)) == Vec2(4.0f, -3.0f));
}

/* toTransformed with the identity matrix returns the point unchanged. */
TEST(Vec2, ToTransformedIdentity) {
    ASSERT_TRUE(Vec2(3.0f, 4.0f).toTransformed(Mat3{}) == Vec2(3.0f, 4.0f));
}

/* toTransformed applies a row-major matrix with translation in the bottom row. */
TEST(Vec2, ToTransformedRowMajor) {
    Mat3 m(2.0f, 0.0f, 0.0f,
           0.0f, 3.0f, 0.0f,
           10.0f, 20.0f, 1.0f);
    ASSERT_TRUE(Vec2(1.0f, 1.0f).toTransformed(m) == Vec2(12.0f, 23.0f));
}
