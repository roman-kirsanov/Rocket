/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Filter.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* BlurFilter equality follows the radius. */
TEST(Filter, BlurFilterEqualityFollowsRadius) {
    BlurFilter a{4.0f};
    BlurFilter b{4.0f};
    BlurFilter c{8.0f};
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(!(a != b));
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(!(a == c));
}

/* ShadowFilter equality covers all four fields. */
TEST(Filter, ShadowFilterEqualityCoversAllFields) {
    ShadowFilter a{4.0f, Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec2(1.0f, 2.0f), Vec2(3.0f, 4.0f)};
    ShadowFilter b{4.0f, Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec2(1.0f, 2.0f), Vec2(3.0f, 4.0f)};
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(!(a != b));

    ShadowFilter radius = a;
    radius.radius = 8.0f;
    ASSERT_TRUE(a != radius);

    ShadowFilter color = a;
    color.color = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    ASSERT_TRUE(a != color);

    ShadowFilter offset = a;
    offset.offset = Vec2(5.0f, 6.0f);
    ASSERT_TRUE(a != offset);

    ShadowFilter spread = a;
    spread.spread = Vec2(5.0f, 6.0f);
    ASSERT_TRUE(a != spread);

    /* Unset optionals differ from set ones and match each other. */
    ShadowFilter unset{4.0f, Vec4(0.0f, 0.0f, 0.0f, 1.0f), std::nullopt, std::nullopt};
    ASSERT_TRUE(a != unset);
    ShadowFilter unset2{4.0f, Vec4(0.0f, 0.0f, 0.0f, 1.0f), std::nullopt, std::nullopt};
    ASSERT_TRUE(unset == unset2);
}

/* A Filter holding a BlurFilter reports and exposes the blur alternative. */
TEST(Filter, HoldsBlurFilterAlternative) {
    Filter filter = BlurFilter{4.0f};
    ASSERT_TRUE(filter.is<BlurFilter>());
    ASSERT_TRUE(!filter.is<ShadowFilter>());
    ASSERT_TRUE(filter.as<BlurFilter>() != nullptr);
    ASSERT_TRUE(filter.as<BlurFilter>()->radius == 4.0f);
    ASSERT_TRUE(filter.as<ShadowFilter>() == nullptr);

    bool blurVisited = false;
    filter.match(
        [&](BlurFilter const&) { blurVisited = true; },
        [&](ShadowFilter const&) {});
    ASSERT_TRUE(blurVisited);
}

/* A Filter holding a ShadowFilter reports and exposes the shadow alternative. */
TEST(Filter, HoldsShadowFilterAlternative) {
    Filter filter = ShadowFilter{2.0f, Vec4(0.0f, 0.0f, 0.0f, 0.5f), Vec2(1.0f, 1.0f), std::nullopt};
    ASSERT_TRUE(filter.is<ShadowFilter>());
    ASSERT_TRUE(!filter.is<BlurFilter>());
    ASSERT_TRUE(filter.as<ShadowFilter>() != nullptr);
    ASSERT_TRUE(filter.as<ShadowFilter>()->radius == 2.0f);
    ASSERT_TRUE(filter.as<BlurFilter>() == nullptr);

    bool shadowVisited = false;
    filter.match(
        [&](BlurFilter const&) {},
        [&](ShadowFilter const&) { shadowVisited = true; });
    ASSERT_TRUE(shadowVisited);
}
