/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <array>
#include <cstdint>
#include <string>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Icon.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static auto const _svg = std::string(R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M4 4h16v16h-16z"/></svg>)");
static auto const _pixels2x1 = std::array<std::uint8_t, 8>{ 255, 0, 0, 255, 0, 0, 255, 255 };

static ImageBrush const* _FindBrush(class Node* node) {
    if (node == nullptr) {
        return nullptr;
    }

    if (node->getBackground().has_value() == false) {
        return nullptr;
    }

    return node->getBackground()->as<ImageBrush>();
}

TEST(Icon, DrawsNothingWithoutData) {
    class Node* node = nullptr;
    auto interface = _Interface([&]{ Icon({ .nodeProps = { .ref = &node } }); });

    interface.render();

    ASSERT_NE(_FindBrush(node), nullptr);
    EXPECT_EQ(_FindBrush(node)->image, nullptr);
}

TEST(Icon, DrawsAnSvg) {
    class Node* node = nullptr;
    auto interface = _Interface([&]{ Icon({ .data = SvgIconData{ _svg }, .nodeProps = { .ref = &node } }); });

    interface.render();

    ASSERT_NE(_FindBrush(node), nullptr);
    EXPECT_NE(_FindBrush(node)->image, nullptr);
}

TEST(Icon, DrawsAnSvgWithoutSmoothing) {
    class Node* node = nullptr;
    auto interface = _Interface([&]{ Icon({ .data = SvgIconData{ _svg }, .nodeProps = { .ref = &node } }); });

    interface.render();

    ASSERT_NE(_FindBrush(node), nullptr);
    EXPECT_EQ(_FindBrush(node)->filterMag, ImageFilter::Nearest);
    EXPECT_EQ(_FindBrush(node)->filterMin, ImageFilter::Nearest);
}

TEST(Icon, DrawsAnSvgDifferentlyForEachIntent) {
    class Node* primary = nullptr;
    class Node* danger = nullptr;
    auto interface = _Interface([&]{
        Icon({ .data = SvgIconData{ _svg }, .intent = IconIntent::Primary, .nodeProps = { .ref = &primary } });
        Icon({ .data = SvgIconData{ _svg }, .intent = IconIntent::Danger, .nodeProps = { .ref = &danger } });
    });

    interface.render();

    ASSERT_NE(_FindBrush(primary), nullptr);
    ASSERT_NE(_FindBrush(danger), nullptr);
    EXPECT_NE(_FindBrush(primary)->image, _FindBrush(danger)->image);
}

TEST(Icon, DrawsTheImageItIsGiven) {
    class Node* node = nullptr;
    auto const image = Image(Vec2(2.0f, 1.0f), _pixels2x1.data());
    auto interface = _Interface([&]{
        Icon({ .data = ImageIconData{ image, ImageFilter::Nearest }, .nodeProps = { .ref = &node } });
    });

    interface.render();

    ASSERT_NE(_FindBrush(node), nullptr);
    EXPECT_EQ(_FindBrush(node)->image, &image);
}

TEST(Icon, DrawsAnImageWithTheFilterItIsGiven) {
    class Node* node = nullptr;
    auto const image = Image(Vec2(2.0f, 1.0f), _pixels2x1.data());
    auto interface = _Interface([&]{
        Icon({ .data = ImageIconData{ image, ImageFilter::Linear }, .nodeProps = { .ref = &node } });
    });

    interface.render();

    ASSERT_NE(_FindBrush(node), nullptr);
    EXPECT_EQ(_FindBrush(node)->filterMag, ImageFilter::Linear);
    EXPECT_EQ(_FindBrush(node)->filterMin, ImageFilter::Linear);
}

TEST(Icon, DrawsTheSameImageWhateverTheIntent) {
    class Node* primary = nullptr;
    class Node* danger = nullptr;
    auto const image = Image(Vec2(2.0f, 1.0f), _pixels2x1.data());
    auto interface = _Interface([&]{
        Icon({ .data = ImageIconData{ image, ImageFilter::Nearest }, .intent = IconIntent::Primary, .nodeProps = { .ref = &primary } });
        Icon({ .data = ImageIconData{ image, ImageFilter::Nearest }, .intent = IconIntent::Danger, .nodeProps = { .ref = &danger } });
    });

    interface.render();

    ASSERT_NE(_FindBrush(primary), nullptr);
    ASSERT_NE(_FindBrush(danger), nullptr);
    EXPECT_EQ(_FindBrush(primary)->image, &image);
    EXPECT_EQ(_FindBrush(danger)->image, &image);
}

TEST(Icon, IsMediumSizedByDefault) {
    class Node* node = nullptr;
    auto interface = _Interface([&]{ Icon({ .data = SvgIconData{ _svg }, .nodeProps = { .ref = &node } }); });

    interface.render();

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getWidth(), NodeValue(16.0f));
    EXPECT_EQ(node->getHeight(), NodeValue(16.0f));
}

TEST(Icon, GivesAnSvgTheSizeAskedFor) {
    class Node* node = nullptr;
    auto interface = _Interface([&]{
        Icon({ .data = SvgIconData{ _svg }, .size = IconSize::Large, .nodeProps = { .ref = &node } });
    });

    interface.render();

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getWidth(), NodeValue(20.0f));
    EXPECT_EQ(node->getHeight(), NodeValue(20.0f));
}

TEST(Icon, GivesAnImageTheSizeAskedForNotItsOwn) {
    class Node* node = nullptr;
    auto const image = Image(Vec2(2.0f, 1.0f), _pixels2x1.data());
    auto interface = _Interface([&]{
        Icon({ .data = ImageIconData{ image, ImageFilter::Nearest }, .size = IconSize::Small, .nodeProps = { .ref = &node } });
    });

    interface.render();

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getWidth(), NodeValue(14.0f));
    EXPECT_EQ(node->getHeight(), NodeValue(14.0f));
}

TEST(Icon, GivesAnImageTheExactSizeAskedFor) {
    class Node* node = nullptr;
    auto const image = Image(Vec2(2.0f, 1.0f), _pixels2x1.data());
    auto interface = _Interface([&]{
        Icon({ .data = ImageIconData{ image, ImageFilter::Nearest }, .sizeValue = Vec2{ 20.0f, 10.0f }, .nodeProps = { .ref = &node } });
    });

    interface.render();

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getWidth(), NodeValue(20.0f));
    EXPECT_EQ(node->getHeight(), NodeValue(10.0f));
}

TEST(Icon, DrawsTheImageWhenItsDataChangesFromAnSvg) {
    class Node* node = nullptr;
    auto const image = Image(Vec2(2.0f, 1.0f), _pixels2x1.data());
    auto useImage = false;
    auto interface = _Interface([&]{
        if (useImage) {
            Icon({ .data = ImageIconData{ image, ImageFilter::Linear }, .nodeProps = { .ref = &node } });
        } else {
            Icon({ .data = SvgIconData{ _svg }, .nodeProps = { .ref = &node } });
        }
    });
    interface.render();

    useImage = true;
    interface.render();

    ASSERT_NE(_FindBrush(node), nullptr);
    EXPECT_EQ(_FindBrush(node)->image, &image);
    EXPECT_EQ(_FindBrush(node)->filterMag, ImageFilter::Linear);
}

TEST(Icon, DrawsTheOtherSvgWhenItIsGivenOne) {
    class Node* node = nullptr;
    auto const other = std::string(R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M8 8h8v8h-8z"/></svg>)");
    auto useOther = false;
    auto interface = _Interface([&]{ Icon({ .data = SvgIconData{ (useOther ? other : _svg) }, .nodeProps = { .ref = &node } }); });
    interface.render();
    ASSERT_NE(_FindBrush(node), nullptr);
    auto const first = _FindBrush(node)->image;

    useOther = true;
    interface.render();

    ASSERT_NE(_FindBrush(node), nullptr);
    EXPECT_NE(_FindBrush(node)->image, first);
}

TEST(Icon, TakesItsSizeFromTheProvidedTheme) {
    class Node* node = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.icon.rules.push_back({ .selector = { .size = IconSize::Medium }, .style = { .size = Vec2{ 18.0f, 18.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Icon({ .data = SvgIconData{ _svg }, .nodeProps = { .ref = &node } });
        });
    });

    interface.render();

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getWidth(), NodeValue(18.0f));
    EXPECT_EQ(node->getHeight(), NodeValue(18.0f));
}
