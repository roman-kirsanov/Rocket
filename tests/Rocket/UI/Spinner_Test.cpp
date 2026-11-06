/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Spinner.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Spinner, DrawsTheThemesDrawingAtItsSize) {
    class Node* medium = nullptr;
    class Node* large = nullptr;
    auto interface = _Interface([&]{
        Spinner({ .nodeProps = { .ref = &medium } });
        Spinner({ .size = SpinnerSize::Large, .nodeProps = { .ref = &large } });
    });

    interface.render();

    ASSERT_NE(medium, nullptr);
    ASSERT_NE(large, nullptr);
    EXPECT_EQ(medium->getWidth(), NodeValue(16.0f));
    EXPECT_EQ(large->getWidth(), NodeValue(20.0f));
    ASSERT_NE(medium->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(medium->getBackground()->as<ImageBrush>()->image, nullptr);
}

TEST(Spinner, TakesItsLookFromTheProvidedTheme) {
    class Node* root = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.spinner.rules.push_back({ .selector = { .size = SpinnerSize::Small }, .style = { .size = Vec2{ 30.0f, 30.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Spinner({ .size = SpinnerSize::Small, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getWidth(), NodeValue(30.0f));
    EXPECT_EQ(root->getHeight(), NodeValue(30.0f));
}

TEST(Spinner, LetsTheCallerOverrideTheNode) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Spinner({ .nodeProps = { .ref = &root, .width = 40.0f } });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getWidth(), NodeValue(40.0f));
}
