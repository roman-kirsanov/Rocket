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
#include <Rocket/Window/WindowEvent.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Tooltip.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static void _MoveTo(_Interface& interface, Vec2 const& position) {
    interface.window.onEvent.publish(MouseMoveWindowEvent(interface.window, position, KeyModifiers{}));
    interface.render();
    interface.render();
    interface.render();
}

static void _Anchor(TooltipProps props, class Node** anchor, float top) {
    props.nodeProps.ref = anchor;
    props.nodeProps.position = NodePosition::Absolute;
    props.nodeProps.left = 200.0f;
    props.nodeProps.top = top;
    props.nodeProps.width = 100.0f;
    props.nodeProps.height = 40.0f;

    Tooltip(props);
}

TEST(Tooltip, WaitsForThePointerToRestBeforeShowing) {
    class Node* anchor = nullptr;
    auto interface = _Interface([&]{
        _Anchor({ .text = "Hint", .delayShow = 100000.0 }, &anchor, 200.0f);
    });
    interface.render();

    _MoveTo(interface, { 250.0f, 220.0f });

    EXPECT_EQ(_FindTexts(anchor), std::vector<std::string>{});
}

TEST(Tooltip, ShowsItsTextAboveTheAnchorAndHidesWhenThePointerLeaves) {
    class Node* anchor = nullptr;
    class Node* popup = nullptr;
    auto interface = _Interface([&]{
        _Anchor({ .text = "Hint", .delayShow = 0.0, .delayHide = 0.0, .popupProps = { .ref = &popup } }, &anchor, 200.0f);
    });
    interface.render();

    _MoveTo(interface, { 250.0f, 220.0f });

    ASSERT_NE(popup, nullptr);
    EXPECT_EQ(_FindTexts(anchor), std::vector<std::string>{ "Hint" });
    EXPECT_LT(popup->getComputedBorderRect().y + popup->getComputedBorderRect().height, 200.0f);
    EXPECT_EQ(popup->getZIndex(), TOOLTIP_ZINDEX);

    _MoveTo(interface, { 10.0f, 10.0f });

    EXPECT_EQ(_FindTexts(anchor), std::vector<std::string>{});
}

TEST(Tooltip, PointsItsArrowAtTheAnchorAcrossTheGap) {
    class Node* popup = nullptr;
    class Node* anchor = nullptr;
    auto interface = _Interface([&]{
        _Anchor({ .text = "Hint", .delayShow = 0.0, .popupProps = { .ref = &popup } }, &anchor, 200.0f);
    });
    interface.render();

    _MoveTo(interface, { 250.0f, 220.0f });

    ASSERT_NE(popup, nullptr);
    auto const arrow = popup->getFirstChild()->getComputedBorderRect();
    auto const box = popup->getComputedBorderRect();
    EXPECT_FLOAT_EQ(arrow.y + arrow.height, 200.0f - 4.0f);
    EXPECT_FLOAT_EQ(arrow.y, box.y + box.height);
    EXPECT_FLOAT_EQ(arrow.x + arrow.width * 0.5f, 250.0f);
}

TEST(Tooltip, ShowsBelowTheAnchorWhenThereIsNoRoomAbove) {
    class Node* popup = nullptr;
    class Node* anchor = nullptr;
    auto interface = _Interface([&]{
        _Anchor({ .text = "Hint", .delayShow = 0.0, .popupProps = { .ref = &popup } }, &anchor, 0.0f);
    });
    interface.render();

    _MoveTo(interface, { 250.0f, 20.0f });

    ASSERT_NE(popup, nullptr);
    auto const arrow = popup->getFirstChild()->getComputedBorderRect();
    EXPECT_GT(popup->getComputedBorderRect().y, 40.0f);
    EXPECT_FLOAT_EQ(arrow.y, 40.0f + 4.0f);
}

TEST(Tooltip, TakesItsLookFromTheProvidedTheme) {
    class Node* popup = nullptr;
    class Node* anchor = nullptr;
    auto theme = GetDefaultLightTheme();
    theme.tooltip.rules.push_back({ .selector = { .intent = TooltipIntent::Danger }, .style = { .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, .fontSize = 20.0f } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            _Anchor({ .text = "Hint", .intent = TooltipIntent::Danger, .delayShow = 0.0, .popupProps = { .ref = &popup } }, &anchor, 200.0f);
        });
    });
    interface.render();

    _MoveTo(interface, { 250.0f, 220.0f });

    ASSERT_NE(popup, nullptr);
    EXPECT_EQ(popup->getBackground()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
    EXPECT_EQ(popup->getFontSize(), 20.0f);
}

TEST(Tooltip, DrawsItselfAndItsArrowWithTheMaterialOfAMaterialTheme) {
    class Node* popup = nullptr;
    class Node* anchor = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            _Anchor({ .text = "Hint", .delayShow = 0.0, .popupProps = { .ref = &popup } }, &anchor, 200.0f);
        });
    });
    interface.render();

    _MoveTo(interface, { 250.0f, 220.0f });

    ASSERT_NE(popup, nullptr);
    ASSERT_NE(popup->getBackground()->as<ImageBrush>(), nullptr);
    ASSERT_NE(popup->getFirstChild()->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(popup->getFirstChild()->getBackground()->as<ImageBrush>()->image, nullptr);
}
