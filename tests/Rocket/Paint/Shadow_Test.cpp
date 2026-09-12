/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Shadow.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* Default-constructed shadows compare equal. */
TEST(Shadow, DefaultConstructedShadowsCompareEqual) {
    ASSERT_TRUE(Shadow{} == Shadow{});
    ASSERT_TRUE(!(Shadow{} != Shadow{}));
}

/* A differing color makes shadows unequal. */
TEST(Shadow, DifferingColorMakesShadowsUnequal) {
    Shadow a;
    Shadow b;
    a.color = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    b.color = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
    ASSERT_TRUE(a != b);
    ASSERT_TRUE(!(a == b));
}

/* A differing offset makes shadows unequal. */
TEST(Shadow, DifferingOffsetMakesShadowsUnequal) {
    Shadow a;
    Shadow b;
    a.offset = Vec2(1.0f, 2.0f);
    b.offset = Vec2(3.0f, 4.0f);
    ASSERT_TRUE(a != b);
}

/* A differing spread makes shadows unequal. */
TEST(Shadow, DifferingSpreadMakesShadowsUnequal) {
    Shadow a;
    Shadow b;
    a.spread = Vec2(1.0f, 1.0f);
    b.spread = Vec2(2.0f, 2.0f);
    ASSERT_TRUE(a != b);
}

/* A differing blur makes shadows unequal. */
TEST(Shadow, DifferingBlurMakesShadowsUnequal) {
    Shadow a;
    Shadow b;
    a.blur = 4.0f;
    b.blur = 8.0f;
    ASSERT_TRUE(a != b);
}

/* An unset field differs from a set one. */
TEST(Shadow, UnsetFieldDiffersFromSetOne) {
    Shadow a;
    Shadow b;
    b.color = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    ASSERT_TRUE(a != b);

    a = Shadow{};
    b = Shadow{};
    b.offset = Vec2(1.0f, 1.0f);
    ASSERT_TRUE(a != b);

    a = Shadow{};
    b = Shadow{};
    b.spread = Vec2(1.0f, 1.0f);
    ASSERT_TRUE(a != b);

    a = Shadow{};
    b = Shadow{};
    b.blur = 1.0f;
    ASSERT_TRUE(a != b);
}

/* Identical set values compare equal. */
TEST(Shadow, IdenticalSetValuesCompareEqual) {
    Shadow a;
    Shadow b;
    a.color = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
    a.offset = Vec2(1.0f, 2.0f);
    a.spread = Vec2(3.0f, 4.0f);
    a.blur = 5.0f;
    b.color = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
    b.offset = Vec2(1.0f, 2.0f);
    b.spread = Vec2(3.0f, 4.0f);
    b.blur = 5.0f;
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(!(a != b));
}
