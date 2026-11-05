/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <cstdint>
#include <vector>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Theme/PixelDark/Material.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static std::vector<std::uint8_t> _GetPixel(std::vector<std::uint8_t> const& data, int width, int x, int y) {
    auto const index = (((y * width) + x) * 4);

    return std::vector<std::uint8_t>(data.begin() + index, data.begin() + index + 4);
}

static std::vector<int> _GetStraightRGB(std::vector<std::uint8_t> const& pixel) {
    auto const unpremultiply = [&](std::size_t channel) {
        return (pixel[3] == 0 ? 0 : static_cast<int>(std::lround((static_cast<float>(pixel[channel]) * 255.0f) / static_cast<float>(pixel[3]))));
    };

    return { unpremultiply(0), unpremultiply(1), unpremultiply(2) };
}

static bool _IsNearColor(std::vector<std::uint8_t> const& pixel, std::vector<std::uint8_t> const& color) {
    auto const straight = _GetStraightRGB(pixel);

    for (auto channel = 0llu; channel < straight.size(); channel++) {
        if (std::abs(straight[channel] - static_cast<int>(color[channel])) > 3) {
            return false;
        }
    }

    return true;
}

static std::vector<std::uint8_t> _GetData(Image const& image) {
    auto data = std::vector<std::uint8_t>();
    image.getData(data);

    return data;
}

TEST(MaterialVariant, DrawsTheBrightnessItIsAskedFor) {
    auto const normal = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .brightness = MaterialBrightness::Normal, .roundness = MaterialRoundness::Large, .scale = 1.0f });
    auto const lighter = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .brightness = MaterialBrightness::Lighter, .roundness = MaterialRoundness::Large, .scale = 1.0f });
    auto const darker = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .brightness = MaterialBrightness::Darker, .roundness = MaterialRoundness::Large, .scale = 1.0f });

    EXPECT_EQ(normal.brush.slice, (Vec4{ 96.0f, 48.0f, 16.0f, 16.0f }));
    EXPECT_EQ(lighter.brush.slice, (Vec4{ 112.0f, 48.0f, 16.0f, 16.0f }));
    EXPECT_EQ(darker.brush.slice, (Vec4{ 128.0f, 48.0f, 16.0f, 16.0f }));
}

TEST(MaterialVariant, DrawsEveryBrightnessWithoutSmoothing) {
    for (auto const brightness : { MaterialBrightness::Normal, MaterialBrightness::Lighter, MaterialBrightness::Darker }) {
        auto const variant = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .brightness = brightness, .roundness = MaterialRoundness::Large, .scale = 1.0f });

        EXPECT_EQ(variant.brush.filterMag, ImageFilter::Nearest);
        EXPECT_EQ(variant.brush.filterMin, ImageFilter::Nearest);
    }
}

TEST(MaterialVariant, StretchesEveryBrightnessAcrossTheNode) {
    for (auto const brightness : { MaterialBrightness::Normal, MaterialBrightness::Lighter, MaterialBrightness::Darker }) {
        auto const variant = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .brightness = brightness, .roundness = MaterialRoundness::Large, .scale = 1.0f });

        EXPECT_EQ(variant.brush.positionX, ImagePosition::Stretch);
        EXPECT_EQ(variant.brush.positionY, ImagePosition::Stretch);
    }
}

TEST(MaterialVariant, TakesItsSlicesFromTheImageDrawnForTheScale) {
    auto const single = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .roundness = MaterialRoundness::Large, .scale = 1.0f });
    auto const doubled = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .roundness = MaterialRoundness::Large, .scale = 2.0f });

    EXPECT_NE(single.brush.image, doubled.brush.image);
    EXPECT_EQ(single.brush.slice, (Vec4{ 96.0f, 48.0f, 16.0f, 16.0f }));
    EXPECT_EQ(doubled.brush.slice, (Vec4{ 192.0f, 96.0f, 32.0f, 32.0f }));
    EXPECT_EQ(doubled.brush.nPatch, (Vec4{ 12.0f, 12.0f, 12.0f, 12.0f }));
}

TEST(MaterialVariant, GivesItsBorderInNodeUnits) {
    auto const single = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .roundness = MaterialRoundness::Large, .scale = 1.0f });
    auto const doubled = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .roundness = MaterialRoundness::Large, .scale = 2.0f });
    auto const between = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .roundness = MaterialRoundness::Large, .scale = 1.25f });

    EXPECT_FLOAT_EQ(single.border, 6.0f);
    EXPECT_FLOAT_EQ(doubled.border, 6.0f);
    EXPECT_FLOAT_EQ(between.border, 4.8f);
}

TEST(MaterialVariant, LeavesOutTheEdgeItIsToldTo) {
    auto const material = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Default, .roundness = MaterialRoundness::Large, .scale = 1.0f, .noLeftBorder = true });

    EXPECT_EQ(material.brush.slice, (Vec4{ 102.0f, 48.0f, 10.0f, 16.0f }));
    EXPECT_EQ(material.brush.nPatch, (Vec4{ 0.0f, 6.0f, 6.0f, 6.0f }));
}

TEST(MaterialVariant, HasNoImageForTheNormalBrightnessOfATransparentMaterial) {
    auto const normal = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Transparent, .intent = MaterialIntent::Default, .roundness = MaterialRoundness::Large, .scale = 1.0f });
    auto const lighter = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Transparent, .intent = MaterialIntent::Default, .brightness = MaterialBrightness::Lighter, .roundness = MaterialRoundness::Large, .scale = 1.0f });

    EXPECT_EQ(normal.brush.image, nullptr);
    EXPECT_NE(lighter.brush.image, nullptr);
}

TEST(MaterialVariant, TakesTheDefaultOfEveryOptionLeftUnset) {
    auto const unset = GetPixelDarkMaterial().getVariant({});
    auto const given = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Default, .intent = MaterialIntent::Default, .brightness = MaterialBrightness::Normal, .roundness = MaterialRoundness::None, .scale = 1.0f });

    EXPECT_NE(unset.brush.image, nullptr);
    EXPECT_EQ(unset.brush, given.brush);
    EXPECT_FLOAT_EQ(unset.border, given.border);
}

TEST(MaterialVariant, GivesEveryCornerTheRoundnessUnlessItSetsItsOwn) {
    auto const shared = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::Large });
    auto const each = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .topLeftRoundness = MaterialRoundness::Large, .topRightRoundness = MaterialRoundness::Large, .bottomLeftRoundness = MaterialRoundness::Large, .bottomRightRoundness = MaterialRoundness::Large });
    auto const overridden = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::None, .topLeftRoundness = MaterialRoundness::Large, .topRightRoundness = MaterialRoundness::Large, .bottomLeftRoundness = MaterialRoundness::Large, .bottomRightRoundness = MaterialRoundness::Large });

    EXPECT_EQ(shared.brush, each.brush);
    EXPECT_EQ(shared.brush, overridden.brush);
}

TEST(MaterialVariant, PutsMixedCornersTogetherFromTheVariantOfEachCorner) {
    auto const mixed = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::None, .topLeftRoundness = MaterialRoundness::Large, .scale = 2.0f });
    auto const none = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::None, .scale = 2.0f });
    auto const large = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::Large, .scale = 2.0f });

    ASSERT_NE(mixed.brush.image, nullptr);
    EXPECT_NE(mixed.brush.image, none.brush.image);
    EXPECT_EQ(mixed.brush.image->getSize(), (Vec2{ 96.0f, 32.0f }));
    EXPECT_EQ(mixed.brush.slice, (Vec4{ 0.0f, 0.0f, 32.0f, 32.0f }));
    EXPECT_EQ(GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .brightness = MaterialBrightness::Darker, .roundness = MaterialRoundness::None, .topLeftRoundness = MaterialRoundness::Large, .scale = 2.0f }).brush.slice, (Vec4{ 64.0f, 0.0f, 32.0f, 32.0f }));
    EXPECT_EQ(mixed.brush.nPatch, (Vec4{ 12.0f, 12.0f, 8.0f, 8.0f }));
    EXPECT_FLOAT_EQ(mixed.border, 6.0f);

    auto const mixedData = _GetData(*mixed.brush.image);
    auto const sheetData = _GetData(*none.brush.image);
    auto const sheetWidth = static_cast<int>(none.brush.image->getSize().width);

    for (auto y = 0; y < 16; y++) {
        for (auto x = 0; x < 16; x++) {
            EXPECT_EQ(_GetPixel(mixedData, 96, x, y), _GetPixel(sheetData, sheetWidth, static_cast<int>(large.brush.slice->x) + x, static_cast<int>(large.brush.slice->y) + y));
            EXPECT_EQ(_GetPixel(mixedData, 96, x + 16, y + 16), _GetPixel(sheetData, sheetWidth, static_cast<int>(none.brush.slice->x) + x + 16, static_cast<int>(none.brush.slice->y) + y + 16));
        }
    }
}

TEST(MaterialVariant, BuildsEachMixOfCornersOncePerSheet) {
    auto const options = MaterialVariantOptions{ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .roundness = MaterialRoundness::Small, .bottomRightRoundness = MaterialRoundness::Large };
    auto const first = GetPixelDarkMaterial().getVariant(options);
    auto const second = GetPixelDarkMaterial().getVariant(options);
    auto doubledOptions = options;
    doubledOptions.scale = 2.0f;
    auto const doubled = GetPixelDarkMaterial().getVariant(doubledOptions);

    EXPECT_EQ(first.brush.image, second.brush.image);
    EXPECT_NE(first.brush.image, doubled.brush.image);
}

TEST(MaterialVariant, LeavesOutTheEdgeOfMixedCornersItIsToldTo) {
    auto const material = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::None, .topLeftRoundness = MaterialRoundness::Large, .noLeftBorder = true });

    EXPECT_EQ(material.brush.slice, (Vec4{ 6.0f, 0.0f, 10.0f, 16.0f }));
    EXPECT_EQ(material.brush.nPatch, (Vec4{ 0.0f, 6.0f, 4.0f, 4.0f }));
}

TEST(MaterialVariant, HasNoImageForTheNormalBrightnessOfATransparentMaterialWithMixedCorners) {
    auto const normal = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Transparent, .roundness = MaterialRoundness::Small, .topLeftRoundness = MaterialRoundness::Large });
    auto const lighter = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Transparent, .brightness = MaterialBrightness::Lighter, .roundness = MaterialRoundness::Small, .topLeftRoundness = MaterialRoundness::Large });

    EXPECT_EQ(normal.brush.image, nullptr);
    EXPECT_NE(lighter.brush.image, nullptr);
}

TEST(MaterialArrowVariant, TakesEachArrowFromTheSheetDrawnForTheScale) {
    auto const up = GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Up, .intent = MaterialIntent::Default, .scale = 1.0f });
    auto const left = GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Left, .intent = MaterialIntent::Default, .scale = 1.0f });
    auto const doubled = GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Right, .intent = MaterialIntent::Default, .scale = 2.0f });

    EXPECT_EQ(up.brush.slice, (Vec4{ 0.0f, 568.0f, 16.0f, 8.0f }));
    EXPECT_EQ(GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Up, .intent = MaterialIntent::Default, .brightness = MaterialBrightness::Darker, .scale = 1.0f }).brush.slice, (Vec4{ 32.0f, 568.0f, 16.0f, 8.0f }));
    EXPECT_EQ(left.brush.slice, (Vec4{ 40.0f, 608.0f, 8.0f, 16.0f }));
    EXPECT_EQ(doubled.brush.slice, (Vec4{ 0.0f, 1184.0f, 16.0f, 32.0f }));
    EXPECT_NE(up.brush.image, doubled.brush.image);
    EXPECT_EQ(up.brush.filterMag, ImageFilter::Nearest);
}

TEST(MaterialArrowVariant, SizesTheArrowToBeDrawnPixelForPixelAtAnyScale) {
    auto const single = GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Down, .intent = MaterialIntent::Default, .scale = 1.0f });
    auto const between = GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Down, .intent = MaterialIntent::Default, .scale = 1.25f });
    auto const doubled = GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Down, .intent = MaterialIntent::Default, .scale = 2.0f });

    EXPECT_EQ(single.size, (Vec2{ 16.0f, 8.0f }));
    EXPECT_FLOAT_EQ(single.overlap, 2.0f);
    EXPECT_FLOAT_EQ(between.size.width, 12.8f);
    EXPECT_FLOAT_EQ(between.overlap, 1.6f);
    EXPECT_EQ(doubled.size, (Vec2{ 16.0f, 8.0f }));
}

TEST(MaterialArrowVariant, GivesAnIntentWithNoArrowsOfItsOwnTheArrowsOfTheDefault) {
    auto const primary = GetPixelDarkMaterial().getArrowVariant({ .direction = MaterialArrowDirection::Up, .intent = MaterialIntent::Primary, .scale = 1.0f });

    EXPECT_EQ(primary.brush.slice, (Vec4{ 0.0f, 568.0f, 16.0f, 8.0f }));
}

TEST(MaterialCircleVariant, RoundsTheWidthToWholePixelsOfTheSheet) {
    auto const even = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 24.0f, .scale = 1.0f });
    auto const odd = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 21.0f, .scale = 1.0f });
    auto const between = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 24.0f, .scale = 1.25f });

    EXPECT_EQ(even.size, (Vec2{ 24.0f, 24.0f }));
    EXPECT_EQ(odd.size, (Vec2{ 22.0f, 22.0f }));
    EXPECT_FLOAT_EQ(between.size.width, 24.0f);
    EXPECT_FLOAT_EQ(between.size.height, 24.0f);
}

TEST(MaterialCircleVariant, DrawsOneTexelPerPixelOfTheSheet) {
    auto const single = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 24.0f, .scale = 1.0f });
    auto const tripled = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 24.0f, .scale = 3.0f });

    ASSERT_NE(single.brush.image, nullptr);
    EXPECT_EQ(single.brush.image->getSize(), (Vec2{ 36.0f, 12.0f }));
    EXPECT_EQ(tripled.brush.image, single.brush.image);
    EXPECT_EQ(tripled.size, (Vec2{ 24.0f, 24.0f }));
}

TEST(MaterialCircleVariant, DrawsTheBrightnessItIsAskedFor) {
    auto const normal = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Bordered, .brightness = MaterialBrightness::Normal, .width = 24.0f });
    auto const lighter = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Bordered, .brightness = MaterialBrightness::Lighter, .width = 24.0f });
    auto const darker = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Bordered, .brightness = MaterialBrightness::Darker, .width = 24.0f });

    EXPECT_EQ(normal.brush.slice, (Vec4{ 0.0f, 0.0f, 12.0f, 12.0f }));
    EXPECT_EQ(lighter.brush.slice, (Vec4{ 12.0f, 0.0f, 12.0f, 12.0f }));
    EXPECT_EQ(darker.brush.slice, (Vec4{ 24.0f, 0.0f, 12.0f, 12.0f }));
    EXPECT_EQ(normal.brush.image, lighter.brush.image);
    EXPECT_EQ(normal.brush.filterMag, ImageFilter::Nearest);
    EXPECT_EQ(normal.brush.filterMin, ImageFilter::Nearest);
    EXPECT_EQ(normal.brush.positionX, ImagePosition::Stretch);
    EXPECT_EQ(normal.brush.positionY, ImagePosition::Stretch);
}

TEST(MaterialCircleVariant, GivesTheBorderOfThePanelOfTheSameKind) {
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 24.0f, .scale = 1.25f });
    auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .scale = 1.25f });

    EXPECT_FLOAT_EQ(circle.border, panel.border);
}

TEST(MaterialCircleVariant, BuildsEachCircleOncePerWidthInPixelsOfTheSheet) {
    auto const first = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .width = 32.0f });
    auto const second = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .width = 32.0f });
    auto const wider = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .width = 40.0f });
    auto const doubled = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .width = 32.0f, .scale = 2.0f });
    auto const between = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .width = 32.0f, .scale = 1.25f });

    EXPECT_EQ(first.brush.image, second.brush.image);
    EXPECT_NE(first.brush.image, wider.brush.image);
    EXPECT_EQ(first.brush.image, doubled.brush.image);
    EXPECT_NE(first.brush.image, between.brush.image);
}

TEST(MaterialCircleVariant, HasNoImageForTheNormalBrightnessOfATransparentMaterial) {
    auto const normal = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Transparent, .width = 24.0f });
    auto const lighter = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Transparent, .brightness = MaterialBrightness::Lighter, .width = 24.0f });

    EXPECT_EQ(normal.brush.image, nullptr);
    EXPECT_NE(lighter.brush.image, nullptr);
}

TEST(MaterialCircleVariant, DrawsTheRingsOfThePanelInItsColors) {
    for (auto const intent : { MaterialIntent::Default, MaterialIntent::Primary, MaterialIntent::Danger }) {
        auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .intent = intent, .width = 32.0f });
        auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = intent });

        auto const circleData = _GetData(*circle.brush.image);
        auto const sheetData = _GetData(*panel.brush.image);
        auto const circleWidth = static_cast<int>(circle.brush.image->getSize().width);
        auto const sheetWidth = static_cast<int>(panel.brush.image->getSize().width);
        auto const panelPixel = [&](int x, int y) {
            return _GetPixel(sheetData, sheetWidth, (static_cast<int>(panel.brush.slice->x) + x), (static_cast<int>(panel.brush.slice->y) + y));
        };

        EXPECT_EQ(_GetPixel(circleData, circleWidth, 0, 0)[3], 0);
        EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 0), panelPixel(8, 0));
        EXPECT_TRUE(_IsNearColor(_GetPixel(circleData, circleWidth, 8, 1), panelPixel(8, 2)));
        EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 8), panelPixel(8, 8));
        EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 14), panelPixel(8, 13));
        EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 15), panelPixel(8, 15));
    }
}

TEST(MaterialCircleVariant, LightsALoweredCircleFromTheTop) {
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .width = 32.0f });
    auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary });

    auto const circleData = _GetData(*circle.brush.image);
    auto const sheetData = _GetData(*panel.brush.image);
    auto const circleWidth = static_cast<int>(circle.brush.image->getSize().width);
    auto const sheetWidth = static_cast<int>(panel.brush.image->getSize().width);
    auto const panelPixel = [&](int x, int y) {
        return _GetPixel(sheetData, sheetWidth, (static_cast<int>(panel.brush.slice->x) + x), (static_cast<int>(panel.brush.slice->y) + y));
    };

    EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 0), panelPixel(8, 0));
    EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 1), panelPixel(8, 2));
    EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 15), panelPixel(8, 15));
    EXPECT_NE(panelPixel(8, 0), panelPixel(8, 15));
}

TEST(MaterialCircleVariant, BlendsTheOuterRingOfALoweredCircleFromDarkToLightDownItsMiddleHalf) {
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary, .width = 64.0f });
    auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Lowered, .intent = MaterialIntent::Primary });

    auto const circleData = _GetData(*circle.brush.image);
    auto const sheetData = _GetData(*panel.brush.image);
    auto const circleWidth = static_cast<int>(circle.brush.image->getSize().width);
    auto const sheetWidth = static_cast<int>(panel.brush.image->getSize().width);
    auto const panelPixel = [&](int x, int y) {
        return _GetPixel(sheetData, sheetWidth, (static_cast<int>(panel.brush.slice->x) + x), (static_cast<int>(panel.brush.slice->y) + y));
    };
    auto const ring = [&](int y) {
        auto x = 0;

        while (_GetPixel(circleData, circleWidth, x, y)[3] == 0) {
            x++;
        }

        return _GetPixel(circleData, circleWidth, x, y);
    };
    auto const brightness = [](std::vector<std::uint8_t> const& pixel) {
        auto const straight = _GetStraightRGB(pixel);

        return (straight[0] + straight[1] + straight[2]);
    };

    EXPECT_TRUE(_IsNearColor(ring(7), panelPixel(8, 0)));
    EXPECT_TRUE(_IsNearColor(ring(25), panelPixel(0, 8)));

    for (auto y = 0; y < 31; y++) {
        EXPECT_LE(brightness(ring(y)), (brightness(ring(y + 1)) + 6)) << "row " << y;
    }

    auto blended = 0;

    for (auto y = 8; y < 24; y++) {
        blended += ((_IsNearColor(ring(y), panelPixel(8, 0)) == false && _IsNearColor(ring(y), panelPixel(0, 8)) == false) ? 1 : 0);
    }

    EXPECT_GT(blended, 8);
}

TEST(MaterialCircleVariant, BlendsTheBevelOfARaisedCircleFromLightToDarkDownItsMiddleHalf) {
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Primary, .width = 64.0f });
    auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Primary });

    auto const circleData = _GetData(*circle.brush.image);
    auto const sheetData = _GetData(*panel.brush.image);
    auto const circleWidth = static_cast<int>(circle.brush.image->getSize().width);
    auto const sheetWidth = static_cast<int>(panel.brush.image->getSize().width);
    auto const panelPixel = [&](int x, int y) {
        return _GetPixel(sheetData, sheetWidth, (static_cast<int>(panel.brush.slice->x) + x), (static_cast<int>(panel.brush.slice->y) + y));
    };
    auto const bevel = [&](int y) {
        auto x = 0;

        while (_GetPixel(circleData, circleWidth, x, y)[3] == 0 || _IsNearColor(_GetPixel(circleData, circleWidth, x, y), panelPixel(8, 0))) {
            x++;
        }

        if (_GetPixel(circleData, circleWidth, x, y) == panelPixel(8, 8)) {
            x++;
        }

        return _GetPixel(circleData, circleWidth, x, y);
    };
    auto const brightness = [](std::vector<std::uint8_t> const& color) {
        return (color[0] + color[1] + color[2]);
    };

    EXPECT_EQ(bevel(7), panelPixel(8, 2));
    EXPECT_EQ(bevel(25), panelPixel(8, 13));

    for (auto y = 16; y < 30; y++) {
        EXPECT_GE(brightness(bevel(y)), brightness(bevel(y + 1))) << "row " << y;
    }

    auto blended = 0;

    for (auto y = 8; y < 24; y++) {
        blended += ((bevel(y) != panelPixel(8, 2) && bevel(y) != panelPixel(8, 13)) ? 1 : 0);
    }

    EXPECT_GT(blended, 8);
}

TEST(MaterialCircleVariant, LeavesNoFillBetweenTheEdgeAndTheShadeOfARaisedCircle) {
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Primary, .width = 64.0f });
    auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Primary });

    auto const circleData = _GetData(*circle.brush.image);
    auto const sheetData = _GetData(*panel.brush.image);
    auto const circleWidth = static_cast<int>(circle.brush.image->getSize().width);
    auto const sheetWidth = static_cast<int>(panel.brush.image->getSize().width);
    auto const panelPixel = [&](int x, int y) {
        return _GetPixel(sheetData, sheetWidth, (static_cast<int>(panel.brush.slice->x) + x), (static_cast<int>(panel.brush.slice->y) + y));
    };

    for (auto y = 16; y < 31; y++) {
        auto x = 0;

        while (_GetPixel(circleData, circleWidth, x, y)[3] == 0 || _IsNearColor(_GetPixel(circleData, circleWidth, x, y), panelPixel(8, 0))) {
            x++;
        }

        EXPECT_NE(_GetPixel(circleData, circleWidth, x, y), panelPixel(8, 8)) << "row " << y;
    }
}

TEST(MaterialCircleVariant, FadesTheLightBevelOfARaisedCircleIntoTheFillAndKeepsItsGaps) {
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 64.0f });
    auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised });

    auto const circleData = _GetData(*circle.brush.image);
    auto const sheetData = _GetData(*panel.brush.image);
    auto const circleWidth = static_cast<int>(circle.brush.image->getSize().width);
    auto const sheetWidth = static_cast<int>(panel.brush.image->getSize().width);
    auto const panelPixel = [&](int x, int y) {
        return _GetPixel(sheetData, sheetWidth, (static_cast<int>(panel.brush.slice->x) + x), (static_cast<int>(panel.brush.slice->y) + y));
    };
    auto const fill = panelPixel(8, 8);
    auto const light = panelPixel(8, 2);

    auto gaps = 0;
    auto faded = 0;

    for (auto y = 1; y < 8; y++) {
        auto x = 0;

        while (_GetPixel(circleData, circleWidth, x, y)[3] == 0 || _IsNearColor(_GetPixel(circleData, circleWidth, x, y), panelPixel(8, 0))) {
            x++;
        }

        gaps += (_GetPixel(circleData, circleWidth, x, y) == fill ? 1 : 0);

        for (auto column = 0; column < 32; column++) {
            auto const pixel = _GetPixel(circleData, circleWidth, column, y);

            faded += ((pixel[3] == 255 && pixel[0] > fill[0] && pixel[0] < light[0]) ? 1 : 0);
        }
    }

    EXPECT_GT(gaps, 0);
    EXPECT_GT(faded, 0);
}

TEST(MaterialCircleVariant, FadesEachEdgePixelByHowMuchOfItTheCircleCovers) {
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .width = 32.0f });
    auto const circleData = _GetData(*circle.brush.image);
    auto const circleWidth = static_cast<int>(circle.brush.image->getSize().width);

    auto faded = 0;

    for (auto y = 0; y < 16; y++) {
        for (auto x = 0; x < 16; x++) {
            auto const alpha = _GetPixel(circleData, circleWidth, x, y)[3];

            faded += ((alpha > 0 && alpha < 230) ? 1 : 0);
        }
    }

    EXPECT_GT(faded, 0);
    EXPECT_EQ(_GetPixel(circleData, circleWidth, 8, 8)[3], 255);
    EXPECT_EQ(_GetPixel(circleData, circleWidth, 0, 8)[3], 255);
}

TEST(MaterialCapsuleVariant, RoundsItsSizeToWholePixelsOfTheSheet) {
    auto const capsule = GetPixelDarkMaterial().getCapsuleVariant({ .kind = MaterialKind::Lowered, .width = 37.0f, .height = 20.0f, .scale = 1.0f });

    ASSERT_NE(capsule.brush.image, nullptr);
    EXPECT_EQ(capsule.size, (Vec2{ 38.0f, 20.0f }));
    EXPECT_EQ(capsule.brush.image->getSize(), (Vec2{ 57.0f, 10.0f }));
    EXPECT_EQ(capsule.brush.slice, (Vec4{ 0.0f, 0.0f, 19.0f, 10.0f }));
}

TEST(MaterialCapsuleVariant, IsACircleWhenAsWideAsItIsHigh) {
    auto const capsule = GetPixelDarkMaterial().getCapsuleVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Primary, .width = 32.0f, .height = 32.0f });
    auto const circle = GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .intent = MaterialIntent::Primary, .width = 32.0f });

    EXPECT_EQ(capsule.brush, circle.brush);
    EXPECT_EQ(capsule.size, circle.size);
    EXPECT_FLOAT_EQ(capsule.border, circle.border);
}

TEST(MaterialCapsuleVariant, RoundsItsShorterSidesAndKeepsItsLongerSidesStraight) {
    auto const capsule = GetPixelDarkMaterial().getCapsuleVariant({ .kind = MaterialKind::Default, .width = 64.0f, .height = 24.0f });
    auto const panel = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Default });

    auto const capsuleData = _GetData(*capsule.brush.image);
    auto const sheetData = _GetData(*panel.brush.image);
    auto const capsuleWidth = static_cast<int>(capsule.brush.image->getSize().width);
    auto const sheetWidth = static_cast<int>(panel.brush.image->getSize().width);
    auto const outline = _GetPixel(sheetData, sheetWidth, (static_cast<int>(panel.brush.slice->x) + 8), static_cast<int>(panel.brush.slice->y));

    for (auto x = 6; x < 26; x++) {
        EXPECT_EQ(_GetPixel(capsuleData, capsuleWidth, x, 0), outline) << "column " << x;
        EXPECT_EQ(_GetPixel(capsuleData, capsuleWidth, x, 11), outline) << "column " << x;
    }

    EXPECT_EQ(_GetPixel(capsuleData, capsuleWidth, 0, 0)[3], 0);
    EXPECT_EQ(_GetPixel(capsuleData, capsuleWidth, 31, 0)[3], 0);
    EXPECT_TRUE(_IsNearColor(_GetPixel(capsuleData, capsuleWidth, 0, 6), outline));
    EXPECT_TRUE(_IsNearColor(_GetPixel(capsuleData, capsuleWidth, 31, 6), outline));
}

TEST(Material, CutsItsBrushesFromTheImageDrawnForTheScale) {
    auto const image1x = Image(Vec2{ 144.0f, 624.0f });
    auto const image2x = Image(Vec2{ 288.0f, 1248.0f });
    auto const image3x = Image(Vec2{ 432.0f, 1872.0f });
    auto const material = Material(image1x, image2x, image3x);

    EXPECT_EQ(material.getVariant({ .scale = 1.0f }).brush.image, &image1x);
    EXPECT_EQ(material.getVariant({ .scale = 2.0f }).brush.image, &image2x);
    EXPECT_EQ(material.getVariant({ .scale = 3.0f }).brush.image, &image3x);
    EXPECT_EQ(material.getArrowVariant({ .direction = MaterialArrowDirection::Up, .intent = MaterialIntent::Default, .scale = 2.0f }).brush.image, &image2x);
}
