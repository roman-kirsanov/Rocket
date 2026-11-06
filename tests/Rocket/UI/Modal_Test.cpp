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
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Modal.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Modal, CoversTheWindowWithItsBackdropAboveTheDocument) {
    class Node* backdrop = nullptr;
    auto interface = _Interface([&]{
        Node({ .width = 100.0f, .height = 100.0f }, [&]{
            Modal({ .backdropProps = { .ref = &backdrop } });
        });
    });

    interface.render();

    ASSERT_NE(backdrop, nullptr);
    EXPECT_EQ(backdrop->getComputedBorderRect(), (Vec4{ 0.0f, 0.0f, 640.0f, 480.0f }));
    EXPECT_EQ(backdrop->getZIndex(), MODAL_ZINDEX);
}

TEST(Modal, CentersItsPanelHoldingTheChildren) {
    class Node* panel = nullptr;
    auto interface = _Interface([&]{
        Modal({ .nodeProps = { .ref = &panel, .width = 200.0f, .height = 100.0f } }, [&]{
            Node({ .display = NodeDisplay::Text, .content = "Hello" });
        });
    });

    interface.render();

    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(panel->getComputedBorderRect(), (Vec4{ 220.0f, 190.0f, 200.0f, 100.0f }));
    EXPECT_EQ(_FindTexts(panel), std::vector<std::string>{ "Hello" });
}

TEST(Modal, TakesItsLookFromTheProvidedTheme) {
    class Node* backdrop = nullptr;
    class Node* panel = nullptr;
    auto theme = GetDefaultLightTheme();
    theme.modal.rules.push_back({ .style = { .backdrop = Vec4{ 0.0f, 0.0f, 1.0f, 0.5f }, .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Modal({ .backdropProps = { .ref = &backdrop }, .nodeProps = { .ref = &panel } });
        });
    });

    interface.render();

    ASSERT_NE(backdrop, nullptr);
    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(backdrop->getBackground()->as<ColorBrush>()->color, (Vec4{ 0.0f, 0.0f, 1.0f, 0.5f }));
    EXPECT_EQ(panel->getBackground()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
}

TEST(Modal, DrawsItsPanelWithTheMaterialOfAMaterialTheme) {
    class Node* panel = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Modal({ .nodeProps = { .ref = &panel } });
        });
    });

    interface.render();

    ASSERT_NE(panel, nullptr);
    ASSERT_NE(panel->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(panel->getBackground()->as<ImageBrush>()->image, nullptr);
}
