/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <Rocket/Window/Input.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Radiobox.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Radiobox, ShowsItsLabelAfterTheBox) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Radiobox({ .label = "Small", .nodeProps = { .ref = &root } });
    });

    interface.render();

    EXPECT_EQ(_FindTexts(root), std::vector<std::string>{ "Small" });
}

TEST(Radiobox, AsksToBeCheckedWhenClickedOrOnSpace) {
    class Node* root = nullptr;
    auto values = std::vector<bool>();
    auto interface = _Interface([&]{
        Radiobox({ .onChange = [&](bool value) { values.push_back(value); }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);
    _PressKey(root, Scancode::Space, Keycode::Space);

    EXPECT_EQ(values, (std::vector<bool>{ true, true }));
}

TEST(Radiobox, StaysCheckedWhenClickedAgain) {
    class Node* root = nullptr;
    auto calls = 0;
    auto interface = _Interface([&]{
        Radiobox({ .checked = true, .onChange = [&](bool) { calls += 1; }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);
    _PressKey(root, Scancode::Space, Keycode::Space);

    EXPECT_EQ(calls, 0);
}

TEST(Radiobox, IgnoresClicksWhenDisabled) {
    class Node* root = nullptr;
    auto calls = 0;
    auto interface = _Interface([&]{
        Radiobox({ .disabled = true, .onChange = [&](bool) { calls += 1; }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);

    EXPECT_EQ(calls, 0);
    EXPECT_EQ(_FindClickableNodes(root).size(), 0);
    EXPECT_EQ(root->getTabIndex(), 0);
}

TEST(Radiobox, DrawsARoundBoxWithADotOnlyWhenChecked) {
    class Node* unchecked = nullptr;
    class Node* checked = nullptr;
    auto interface = _Interface([&]{
        Radiobox({ .boxProps = { .ref = &unchecked } });
        Radiobox({ .checked = true, .boxProps = { .ref = &checked } });
    });

    interface.render();

    ASSERT_NE(unchecked, nullptr);
    ASSERT_NE(checked, nullptr);
    EXPECT_EQ(unchecked->getWidth(), NodeValue(18.0f));
    EXPECT_EQ(unchecked->getBorderTopLeftRadius(), 9.0f);
    EXPECT_EQ(unchecked->getFirstChild(), nullptr);
    EXPECT_NE(checked->getFirstChild(), nullptr);
}

TEST(Radiobox, TakesItsLookFromTheProvidedTheme) {
    class Node* box = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.radiobox.rules.push_back({ .selector = { .checked = true }, .style = { .boxSize = 26.0f, .dotBackground = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, .dotSize = 12.0f } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Radiobox({ .checked = true, .boxProps = { .ref = &box } });
        });
    });

    interface.render();

    ASSERT_NE(box, nullptr);
    ASSERT_NE(box->getFirstChild(), nullptr);
    EXPECT_EQ(box->getWidth(), NodeValue(26.0f));
    EXPECT_EQ(box->getFirstChild()->getWidth(), NodeValue(12.0f));
    EXPECT_EQ(box->getFirstChild()->getBackground()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
}

TEST(Radiobox, DrawsItsBoxAndDotWithMaterialCirclesOfAMaterialTheme) {
    class Node* box = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Radiobox({ .checked = true, .boxProps = { .ref = &box } });
        });
    });

    interface.render();

    ASSERT_NE(box, nullptr);
    ASSERT_NE(box->getFirstChild(), nullptr);
    EXPECT_NE(box->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(box->getFirstChild()->getBackground()->as<ImageBrush>(), nullptr);
}
