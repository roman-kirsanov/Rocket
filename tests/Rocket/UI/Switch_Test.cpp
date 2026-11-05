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
#include <Rocket/UI/Switch.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Switch, ShowsItsLabelAfterTheTrack) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Switch({ .label = "Wi-Fi", .nodeProps = { .ref = &root } });
    });

    interface.render();

    EXPECT_EQ(_FindTexts(root), std::vector<std::string>{ "Wi-Fi" });
}

TEST(Switch, AsksToTakeTheOppositeValueWhenClickedOrOnSpace) {
    class Node* root = nullptr;
    auto checked = false;
    auto values = std::vector<bool>();
    auto interface = _Interface([&]{
        Switch({ .checked = checked, .onChange = [&](bool value) { values.push_back(value); }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);
    checked = true;
    interface.render();
    _PressKey(root, Scancode::Space, Keycode::Space);

    EXPECT_EQ(values, (std::vector<bool>{ true, false }));
}

TEST(Switch, IgnoresClicksWhenDisabled) {
    class Node* root = nullptr;
    auto calls = 0;
    auto interface = _Interface([&]{
        Switch({ .disabled = true, .onChange = [&](bool) { calls += 1; }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);

    EXPECT_EQ(calls, 0);
    EXPECT_EQ(_FindClickableNodes(root).size(), 0);
    EXPECT_EQ(root->getTabIndex(), 0);
}

TEST(Switch, PutsTheKnobAtTheEndOfTheTrackWhenOn) {
    class Node* off = nullptr;
    class Node* on = nullptr;
    auto interface = _Interface([&]{
        Switch({ .trackProps = { .ref = &off } });
        Switch({ .checked = true, .trackProps = { .ref = &on } });
    });

    interface.render();

    ASSERT_NE(off, nullptr);
    ASSERT_NE(on, nullptr);
    EXPECT_EQ(off->getJustify(), NodeJustify::Start);
    EXPECT_EQ(on->getJustify(), NodeJustify::End);
    EXPECT_NE(on->getFirstChild(), nullptr);
}

TEST(Switch, SpacesTheKnobAsFarFromTheEndsAsFromTheSides) {
    class Node* track = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.switch_.rules.push_back({ .style = { .trackSize = Vec2{ 40.0f, 20.0f }, .knobSize = 12.0f } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Switch({ .trackProps = { .ref = &track } });
        });
    });

    interface.render();

    ASSERT_NE(track, nullptr);
    EXPECT_EQ(track->getWidth(), NodeValue(40.0f));
    EXPECT_EQ(track->getHeight(), NodeValue(20.0f));
    EXPECT_EQ(track->getPaddingLeft(), NodeValue(4.0f));
    EXPECT_EQ(track->getPaddingRight(), NodeValue(4.0f));
    EXPECT_EQ(track->getBorderTopLeftRadius(), 10.0f);
}

TEST(Switch, TakesItsLookFromTheProvidedTheme) {
    class Node* track = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.switch_.rules.push_back({ .selector = { .intent = SwitchIntent::Success, .checked = true }, .style = { .background = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Switch({ .checked = true, .intent = SwitchIntent::Success, .trackProps = { .ref = &track } });
        });
    });

    interface.render();

    ASSERT_NE(track, nullptr);
    EXPECT_EQ(track->getBackground()->as<ColorBrush>()->color, (Vec4{ 0.0f, 1.0f, 0.0f, 1.0f }));
}

TEST(Switch, DrawsACapsuleTrackAndACircleKnobWithAMaterialTheme) {
    class Node* track = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Switch({ .trackProps = { .ref = &track } });
        });
    });

    interface.render();

    ASSERT_NE(track, nullptr);
    ASSERT_NE(track->getFirstChild(), nullptr);
    ASSERT_NE(track->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(track->getBackground()->as<ImageBrush>()->image, nullptr);
    EXPECT_NE(track->getFirstChild()->getBackground()->as<ImageBrush>(), nullptr);
}
