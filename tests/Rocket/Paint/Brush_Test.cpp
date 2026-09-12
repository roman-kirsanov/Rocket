/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Brush.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* Brush equality drives repaint and cache invalidation on Node (setters
   compare before invalidating), so a field missing from an operator== fails
   silently as missed repaints. These tests flip every field one at a time. */

TEST(Brush, ColorBrushEquality) {
    ASSERT_TRUE(ColorBrush{} == ColorBrush{});
    ASSERT_TRUE(!(ColorBrush{} != ColorBrush{}));

    auto const a = ColorBrush{ .color = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } };
    auto const b = ColorBrush{ .color = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } };
    ASSERT_TRUE(a != b);
    ASSERT_TRUE(!(a == b));
    ASSERT_TRUE((a == ColorBrush{ .color = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } }));
}

TEST(Brush, ImageBrushEqualityCoversEveryField) {
    auto imageA = Image{ Vec2{ 1.0f, 1.0f } };
    auto imageB = Image{ Vec2{ 1.0f, 1.0f } };

    ASSERT_TRUE(ImageBrush{} == ImageBrush{});
    ASSERT_TRUE(!(ImageBrush{} != ImageBrush{}));

    auto const base = ImageBrush{ .image = &imageA };
    ASSERT_TRUE(base == ImageBrush{ .image = &imageA });

    auto const differing = std::vector<ImageBrush>{
        { .image = &imageB },
        { .image = &imageA, .slice = Vec4{ 0.0f, 0.0f, 1.0f, 1.0f } },
        { .image = &imageA, .nPatch = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f } },
        { .image = &imageA, .color = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } },
        { .image = &imageA, .positionX = ImagePosition::Start },
        { .image = &imageA, .positionY = ImagePosition::End },
        { .image = &imageA, .filterMag = ImageFilter::Linear },
        { .image = &imageA, .filterMin = ImageFilter::Linear },
        { .image = &imageA, .repeatX = true },
        { .image = &imageA, .repeatY = true },
        { .image = &imageA, .flipX = true },
        { .image = &imageA, .flipY = true },
        { .image = &imageA, .fit = true },
    };

    for (auto const& brush : differing) {
        ASSERT_TRUE(base != brush);
        ASSERT_TRUE(!(base == brush));
    }
}

TEST(Brush, GradientBrushEqualityCoversEveryField) {
    ASSERT_TRUE(GradientBrush{} == GradientBrush{});
    ASSERT_TRUE(!(GradientBrush{} != GradientBrush{}));

    auto const differing = std::vector<GradientBrush>{
        { .radial = true },
        { .startPosition = Vec2{ 0.1f, 0.1f } },
        { .stopPosition = Vec2{ 0.9f, 0.9f } },
        { .startColor = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } },
        { .stopColor = Vec4{ 0.0f, 0.0f, 1.0f, 1.0f } },
        { .stops = { { 0.5f, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f } } } },
    };

    for (auto const& brush : differing) {
        ASSERT_TRUE(GradientBrush{} != brush);
        ASSERT_TRUE(!(GradientBrush{} == brush));
    }

    /* stops differing only by position or color, same length */
    auto const stopsA = GradientBrush{ .stops = { { 0.2f, Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } } };
    auto const stopsB = GradientBrush{ .stops = { { 0.8f, Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } } };
    auto const stopsC = GradientBrush{ .stops = { { 0.2f, Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } } } };
    ASSERT_TRUE(stopsA != stopsB);
    ASSERT_TRUE(stopsA != stopsC);
    ASSERT_TRUE((stopsA == GradientBrush{ .stops = { { 0.2f, Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } } }));
}
