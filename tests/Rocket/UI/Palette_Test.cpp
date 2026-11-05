/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/UI/Theme/Default/Palette.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Palette, HoldsTheShadesTailwindDefinesAsTheyAre) {
    auto const& red500 = GetDefaultPalette().red500;

    EXPECT_NEAR(red500.red, 251.0f / 255.0f, 0.002f);
    EXPECT_NEAR(red500.green, 44.0f / 255.0f, 0.002f);
    EXPECT_NEAR(red500.blue, 54.0f / 255.0f, 0.002f);
    EXPECT_EQ(red500.alpha, 1.0f);
}

TEST(Palette, PutsEachInBetweenShadeBetweenItsNeighbors) {
    auto const& palette = GetDefaultPalette();

    EXPECT_LT(palette.neutral150.red, palette.neutral100.red);
    EXPECT_GT(palette.neutral150.red, palette.neutral200.red);
    EXPECT_LT(palette.blue650.blue, palette.blue600.blue);
    EXPECT_GT(palette.blue650.blue, palette.blue700.blue);
}
