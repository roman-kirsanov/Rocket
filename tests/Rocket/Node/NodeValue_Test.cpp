/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <variant>
#include <Rocket/Node/NodeValue.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* A bare float constructs a PixelValue. */
TEST(NodeValue, FloatConstructsPixelValue) {
    NodeValue value = 42.0f;
    auto* pixel = std::get_if<PixelValue>(&value);
    ASSERT_TRUE(pixel != nullptr);
    ASSERT_TRUE(pixel->value == 42.0f);
    ASSERT_TRUE(std::get_if<PercentValue>(&value) == nullptr);
}

/* PercentValue{n} constructs a percentage. */
TEST(NodeValue, PercentValueConstructsPercentage) {
    NodeValue value = PercentValue{50.0f};
    auto* percent = std::get_if<PercentValue>(&value);
    ASSERT_TRUE(percent != nullptr);
    ASSERT_TRUE(percent->value == 50.0f);
    ASSERT_TRUE(std::get_if<PixelValue>(&value) == nullptr);
}

/* Equality compares within the same alternative. */
TEST(NodeValue, EqualityWithinAlternative) {
    ASSERT_TRUE(PixelValue{10.0f} == PixelValue{10.0f});
    ASSERT_TRUE(PixelValue{10.0f} != PixelValue{20.0f});
    ASSERT_TRUE(PercentValue{10.0f} == PercentValue{10.0f});
    ASSERT_TRUE(PercentValue{10.0f} != PercentValue{20.0f});
}

/* NodeValue equality distinguishes pixels from percentages. */
TEST(NodeValue, EqualityDistinguishesAlternatives) {
    NodeValue pixels = 10.0f;
    NodeValue percent = PercentValue{10.0f};
    ASSERT_TRUE(pixels == NodeValue(10.0f));
    ASSERT_TRUE(!(pixels == percent));
}
