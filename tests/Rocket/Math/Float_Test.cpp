/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Math/Float.hpp>
#include <cmath>
#include <gtest/gtest.h>

using namespace Rocket;

/* PI holds the expected value of the mathematical constant. */
TEST(Float, PiValue) {
    ASSERT_TRUE(std::abs(PI - 3.14159265f) < 1e-6f);
}

/* Identical values compare equal. */
TEST(Float, IdenticalValuesEqual) {
    ASSERT_TRUE(FloatIsEqualEps(0.0f, 0.0f));
    ASSERT_TRUE(FloatIsEqualEps(1.0f, 1.0f));
    ASSERT_TRUE(FloatIsEqualEps(-1.0f, -1.0f));
}

/* Positive and negative zero compare equal. */
TEST(Float, SignedZerosEqual) {
    ASSERT_TRUE(FloatIsEqualEps(0.0f, -0.0f));
}

/* Values within the default epsilon (1e-4) compare equal. */
TEST(Float, WithinDefaultEpsilonEqual) {
    ASSERT_TRUE(FloatIsEqualEps(1.0f, 1.00005f));
    ASSERT_TRUE(FloatIsEqualEps(1.00005f, 1.0f));
    ASSERT_TRUE(FloatIsEqualEps(-1.0f, -1.00005f));
    ASSERT_TRUE(FloatIsEqualEps(0.00001f, -0.00001f));
}

/* Values beyond the default epsilon compare unequal. */
TEST(Float, BeyondDefaultEpsilonUnequal) {
    ASSERT_TRUE(!FloatIsEqualEps(1.0f, 1.001f));
    ASSERT_TRUE(!FloatIsEqualEps(1.001f, 1.0f));
    ASSERT_TRUE(!FloatIsEqualEps(-1.0f, -1.001f));
    ASSERT_TRUE(!FloatIsEqualEps(0.001f, -0.001f));
    ASSERT_TRUE(!FloatIsEqualEps(1.0f, 2.0f));
}

/* A custom epsilon widens or narrows the tolerance. */
TEST(Float, CustomEpsilon) {
    ASSERT_TRUE(FloatIsEqualEps(1.0f, 2.0f, 1.5f));
    ASSERT_TRUE(!FloatIsEqualEps(1.0f, 3.0f, 1.5f));
    ASSERT_TRUE(!FloatIsEqualEps(1.0f, 1.00005f, 1e-6f));
}

/* The epsilon bound is inclusive (difference == eps is equal). */
TEST(Float, EpsilonBoundInclusive) {
    ASSERT_TRUE(FloatIsEqualEps(0.0f, 0.5f, 0.5f));
}
