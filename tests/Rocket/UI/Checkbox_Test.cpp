/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <optional>
#include <string>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <Rocket/Window/Input.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Checkbox.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Checkbox, ShowsItsLabelAfterTheBox) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Checkbox({ .label = "Remember me", .nodeProps = { .ref = &root } });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(root->getFirstChild(), nullptr);
    EXPECT_EQ(root->getFirstChild()->getContent(), std::nullopt);
    EXPECT_EQ(_FindTexts(root), std::vector<std::string>{ "Remember me" });
}

TEST(Checkbox, AsksToTakeTheOppositeValueWhenClicked) {
    class Node* root = nullptr;
    auto checked = false;
    auto values = std::vector<bool>();
    auto interface = _Interface([&]{
        Checkbox({ .checked = checked, .onChange = [&](bool value) { values.push_back(value); }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);
    checked = true;
    interface.render();
    _Click(root);

    EXPECT_EQ(values, (std::vector<bool>{ true, false }));
}

TEST(Checkbox, TogglesOnSpace) {
    class Node* root = nullptr;
    auto values = std::vector<bool>();
    auto interface = _Interface([&]{
        Checkbox({ .onChange = [&](bool value) { values.push_back(value); }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _PressKey(root, Scancode::Space, Keycode::Space);
    _PressKey(root, Scancode::Enter, Keycode::Enter);

    EXPECT_EQ(values, (std::vector<bool>{ true }));
}

TEST(Checkbox, IgnoresClicksAndKeysWhenDisabled) {
    class Node* root = nullptr;
    auto calls = 0;
    auto interface = _Interface([&]{
        Checkbox({ .disabled = true, .onChange = [&](bool) { calls += 1; }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);
    _PressKey(root, Scancode::Space, Keycode::Space);

    EXPECT_EQ(calls, 0);
}

TEST(Checkbox, InvitesAClickAndTakesFocusOnlyWhenEnabled) {
    class Node* enabled = nullptr;
    class Node* disabled = nullptr;
    auto interface = _Interface([&]{
        Checkbox({ .nodeProps = { .ref = &enabled } });
        Checkbox({ .disabled = true, .nodeProps = { .ref = &disabled } });
    });

    interface.render();

    ASSERT_NE(enabled, nullptr);
    ASSERT_NE(disabled, nullptr);
    EXPECT_EQ(_FindClickableNodes(enabled).size(), 1);
    EXPECT_EQ(_FindClickableNodes(disabled).size(), 0);
    EXPECT_GT(enabled->getTabIndex(), 0);
    EXPECT_EQ(disabled->getTabIndex(), 0);
}

TEST(Checkbox, DrawsItsMarkOnlyWhenChecked) {
    class Node* unchecked = nullptr;
    class Node* checked = nullptr;
    auto interface = _Interface([&]{
        Checkbox({ .boxProps = { .ref = &unchecked } });
        Checkbox({ .checked = true, .boxProps = { .ref = &checked } });
    });

    interface.render();

    ASSERT_NE(unchecked, nullptr);
    ASSERT_NE(checked, nullptr);
    EXPECT_EQ(unchecked->getFirstChild(), nullptr);
    ASSERT_NE(checked->getFirstChild(), nullptr);
    EXPECT_NE(checked->getFirstChild()->getBackground()->as<ImageBrush>(), nullptr);
}

TEST(Checkbox, TakesItsLookFromTheProvidedTheme) {
    class Node* root = nullptr;
    class Node* box = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.checkbox.rules.push_back({ .selector = { .intent = CheckboxIntent::Danger, .checked = true }, .style = { .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } });
    theme.checkbox.rules.push_back({ .selector = { .size = CheckboxSize::Large }, .style = { .boxSize = 30.0f } });
    theme.checkbox.rules.push_back({ .selector = { .state = CheckboxTheme::State::Disabled }, .style = { .opacity = 0.3f } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Checkbox({ .checked = true, .intent = CheckboxIntent::Danger, .size = CheckboxSize::Large, .disabled = true, .boxProps = { .ref = &box }, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(box, nullptr);
    EXPECT_EQ(root->getOpacity(), 0.3f);
    EXPECT_EQ(box->getWidth(), NodeValue(30.0f));
    EXPECT_EQ(box->getHeight(), NodeValue(30.0f));
    EXPECT_EQ(box->getBackground()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
}

TEST(Checkbox, DrawsItsBoxWithTheMaterialOfAMaterialTheme) {
    class Node* box = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Checkbox({ .boxProps = { .ref = &box } });
        });
    });

    interface.render();

    ASSERT_NE(box, nullptr);
    ASSERT_NE(box->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(box->getBackground()->as<ImageBrush>()->image, nullptr);
}

TEST(Checkbox, LetsTheCallerOverrideTheBox) {
    class Node* box = nullptr;
    auto interface = _Interface([&]{
        Checkbox({ .boxProps = { .ref = &box, .width = 40.0f } });
    });

    interface.render();

    ASSERT_NE(box, nullptr);
    EXPECT_EQ(box->getWidth(), NodeValue(40.0f));
}
