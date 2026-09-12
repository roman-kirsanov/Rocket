/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Math/Mat3.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <cmath>
#include <gtest/gtest.h>

using namespace Rocket;

/* The default constructor produces the identity matrix. */
TEST(Mat3, DefaultConstructorIdentity) {
    Mat3 m;
    ASSERT_TRUE(m == Mat3(1.0f, 0.0f, 0.0f,
                          0.0f, 1.0f, 0.0f,
                          0.0f, 0.0f, 1.0f));
    ASSERT_TRUE(Vec2(3.0f, 4.0f).toTransformed(m) == Vec2(3.0f, 4.0f));
}

/* The element constructor stores values in row-major order in data[]. */
TEST(Mat3, ElementConstructorRowMajor) {
    Mat3 m(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    for (int i = 0; i < 9; i++) {
        ASSERT_TRUE(m.data[i] == float(i + 1));
    }
}

/* Equality compares all nine elements; inequality is its negation. */
TEST(Mat3, EqualityInequality) {
    ASSERT_TRUE(Mat3{} == Mat3{});
    ASSERT_TRUE(!(Mat3{} != Mat3{}));

    Mat3 m;
    m.data[5] = 2.0f;
    ASSERT_TRUE(m != Mat3{});
    ASSERT_TRUE(!(m == Mat3{}));
}

/* Multiplying by the identity leaves a matrix unchanged. */
TEST(Mat3, MultiplyByIdentity) {
    Mat3 m(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    ASSERT_TRUE(m * Mat3{} == m);
    ASSERT_TRUE(Mat3{} * m == m);

    Mat3 n = m;
    n *= Mat3{};
    ASSERT_TRUE(n == m);
}

/* Matrix multiplication matches the hand-computed row-major product. */
TEST(Mat3, MultiplicationRowMajorProduct) {
    Mat3 t(1.0f, 0.0f, 0.0f,
           0.0f, 1.0f, 0.0f,
           1.0f, 2.0f, 1.0f);
    Mat3 s(2.0f, 0.0f, 0.0f,
           0.0f, 3.0f, 0.0f,
           0.0f, 0.0f, 1.0f);
    ASSERT_TRUE(t * s == Mat3(2.0f, 0.0f, 0.0f,
                              0.0f, 3.0f, 0.0f,
                              2.0f, 6.0f, 1.0f));
    ASSERT_TRUE(s * t == Mat3(2.0f, 0.0f, 0.0f,
                              0.0f, 3.0f, 0.0f,
                              1.0f, 2.0f, 1.0f));
}

/* toTranslated appends a translation that offsets transformed points. */
TEST(Mat3, ToTranslated) {
    auto m = Mat3{}.toTranslated(Vec2(10.0f, 20.0f));
    ASSERT_TRUE(m == Mat3(1.0f, 0.0f, 0.0f,
                          0.0f, 1.0f, 0.0f,
                          10.0f, 20.0f, 1.0f));
    ASSERT_TRUE(Vec2(1.0f, 2.0f).toTransformed(m) == Vec2(11.0f, 22.0f));
}

/* toScaled appends a per-axis scale. */
TEST(Mat3, ToScaled) {
    auto m = Mat3{}.toScaled(Vec2(2.0f, 3.0f));
    ASSERT_TRUE(Vec2(1.0f, 1.0f).toTransformed(m) == Vec2(2.0f, 3.0f));
    ASSERT_TRUE(Vec2(4.0f, 5.0f).toTransformed(m) == Vec2(8.0f, 15.0f));
}

/* toRotated by 90 degrees maps (1,0) to (0,1); 360 degrees is a no-op. */
TEST(Mat3, ToRotated) {
    auto p = Vec2(1.0f, 0.0f).toTransformed(Mat3{}.toRotated(90.0f));
    ASSERT_TRUE(std::abs(p.x - 0.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(p.y - 1.0f) < 1e-5f);

    auto q = Vec2(3.0f, 4.0f).toTransformed(Mat3{}.toRotated(360.0f));
    ASSERT_TRUE(std::abs(q.x - 3.0f) < 1e-5f);
    ASSERT_TRUE(std::abs(q.y - 4.0f) < 1e-5f);
}

/* Appended transforms apply in order: scale first, then translation. */
TEST(Mat3, AppendedTransformOrder) {
    auto m = Mat3{}.toScaled(Vec2(2.0f, 3.0f)).toTranslated(Vec2(10.0f, 20.0f));
    ASSERT_TRUE(Vec2(1.0f, 1.0f).toTransformed(m) == Vec2(12.0f, 23.0f));
}

/* OrthoTopLeft maps the viewport to NDC with the origin at top-left (Y down). */
TEST(Mat3, OrthoTopLeft) {
    auto m = Mat3::OrthoTopLeft(Vec4(0.0f, 0.0f, 2.0f, 2.0f));
    ASSERT_TRUE(Vec2(0.0f, 0.0f).toTransformed(m) == Vec2(-1.0f, 1.0f));
    ASSERT_TRUE(Vec2(2.0f, 2.0f).toTransformed(m) == Vec2(1.0f, -1.0f));
    ASSERT_TRUE(Vec2(1.0f, 1.0f).toTransformed(m) == Vec2(0.0f, 0.0f));
}

/* OrthoTopLeft honors a viewport with a non-zero origin. */
TEST(Mat3, OrthoTopLeftNonZeroOrigin) {
    auto m = Mat3::OrthoTopLeft(Vec4(10.0f, 20.0f, 2.0f, 2.0f));
    ASSERT_TRUE(Vec2(10.0f, 20.0f).toTransformed(m) == Vec2(-1.0f, 1.0f));
    ASSERT_TRUE(Vec2(12.0f, 22.0f).toTransformed(m) == Vec2(1.0f, -1.0f));
    ASSERT_TRUE(Vec2(11.0f, 21.0f).toTransformed(m) == Vec2(0.0f, 0.0f));
}

/* OrthoBottomLeft maps the viewport to NDC with the origin at bottom-left (Y up). */
TEST(Mat3, OrthoBottomLeft) {
    auto m = Mat3::OrthoBottomLeft(Vec4(0.0f, 0.0f, 2.0f, 2.0f));
    ASSERT_TRUE(Vec2(0.0f, 0.0f).toTransformed(m) == Vec2(-1.0f, -1.0f));
    ASSERT_TRUE(Vec2(2.0f, 2.0f).toTransformed(m) == Vec2(1.0f, 1.0f));
    ASSERT_TRUE(Vec2(1.0f, 1.0f).toTransformed(m) == Vec2(0.0f, 0.0f));
}
