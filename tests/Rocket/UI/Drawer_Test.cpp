/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <Rocket/Window/Input.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Drawer.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static void _Press(class Node* node) {
    node->triggerEvent(MouseDownNodeEvent(*node, Mouse::LeftButton, { 0.0f, 0.0f }, KeyModifiers{}));
}

TEST(Drawer, RendersNothingUntilVisible) {
    class Node* panel = nullptr;
    auto visible = false;
    auto interface = _Interface([&]{
        Drawer({ .visible = visible, .nodeProps = { .ref = &panel } });
    });

    interface.render();

    EXPECT_EQ(panel, nullptr);

    visible = true;
    interface.render();

    EXPECT_NE(panel, nullptr);
}

TEST(Drawer, PlacesItsPanelAgainstTheEdgeOfTheWindow) {
    class Node* left = nullptr;
    class Node* bottom = nullptr;
    auto interface = _Interface([&]{
        Drawer({ .visible = true, .nodeProps = { .ref = &left, .width = 200.0f } });
        Drawer({ .origin = DrawerOrigin::Bottom, .visible = true, .nodeProps = { .ref = &bottom, .height = 100.0f } });
    });

    interface.render();

    ASSERT_NE(left, nullptr);
    ASSERT_NE(bottom, nullptr);
    EXPECT_EQ(left->getComputedBorderRect(), (Vec4{ 0.0f, 0.0f, 200.0f, 480.0f }));
    EXPECT_EQ(bottom->getComputedBorderRect(), (Vec4{ 0.0f, 380.0f, 640.0f, 100.0f }));
}

TEST(Drawer, AsksToCloseOnAPressOnTheBackdropOnly) {
    class Node* backdrop = nullptr;
    class Node* panel = nullptr;
    auto closes = 0;
    auto interface = _Interface([&]{
        Drawer({ .visible = true, .onClose = [&]{ closes += 1; }, .backdropProps = { .ref = &backdrop }, .nodeProps = { .ref = &panel } });
    });
    interface.render();

    _Press(panel);
    _Press(backdrop);

    EXPECT_EQ(closes, 1);
}

TEST(Drawer, TakesItsLookFromTheProvidedTheme) {
    class Node* panel = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.drawer.rules.push_back({ .selector = { .origin = DrawerOrigin::Right }, .style = { .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, .borderWidth = 3.0f } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Drawer({ .origin = DrawerOrigin::Right, .visible = true, .nodeProps = { .ref = &panel } });
        });
    });

    interface.render();

    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(panel->getBackground()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
    EXPECT_EQ(panel->getBorderLeftWidth(), 3.0f);
    EXPECT_EQ(panel->getBorderRightWidth(), 0.0f);
}

TEST(Drawer, DrawsItsPanelWithTheMaterialOfAMaterialTheme) {
    class Node* panel = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Drawer({ .visible = true, .nodeProps = { .ref = &panel } });
        });
    });

    interface.render();

    ASSERT_NE(panel, nullptr);
    ASSERT_NE(panel->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(panel->getBackground()->as<ImageBrush>()->image, nullptr);
}
