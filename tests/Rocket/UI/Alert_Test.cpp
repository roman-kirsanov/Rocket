/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Button.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Alert.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Alert, ShowsItsTitleBodyAndButtonsInAModal) {
    class Node* panel = nullptr;
    class Node* backdrop = nullptr;
    auto clicks = 0;
    auto interface = _Interface([&]{
        Alert({
            .title = "Delete file?",
            .body = "It cannot be undone.",
            .buttons = [&]{
                Button({ .label = "Delete", .onClick = [&]{ clicks += 1; } });
            },
            .backdropProps = { .ref = &backdrop },
            .nodeProps = { .ref = &panel }
        });
    });

    interface.render();

    ASSERT_NE(panel, nullptr);
    ASSERT_NE(backdrop, nullptr);
    EXPECT_EQ(panel->getParent(), backdrop);
    EXPECT_EQ(_FindTexts(panel), (std::vector<std::string>{ "Delete file?", "It cannot be undone.", "Delete" }));

    _Click(_FindClickableNodes(panel).at(0));

    EXPECT_EQ(clicks, 1);
}

TEST(Alert, LeavesOutWhatItIsNotGiven) {
    class Node* panel = nullptr;
    auto interface = _Interface([&]{
        Alert({ .body = "Done.", .nodeProps = { .ref = &panel } });
    });

    interface.render();

    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(_FindTexts(panel), std::vector<std::string>{ "Done." });
    EXPECT_EQ(panel->getFirstChild()->getNextSibling(), nullptr);
}

TEST(Alert, ShowsTheThemesIconOnlyWhenAskedTo) {
    class Node* icon = nullptr;
    auto interface = _Interface([&]{
        Alert({ .intent = AlertIntent::Warning, .showIcon = true, .title = "Careful", .iconProps = { .nodeProps = { .ref = &icon } } });
    });

    interface.render();

    ASSERT_NE(icon, nullptr);
    EXPECT_NE(icon->getBackground()->as<ImageBrush>(), nullptr);
}

TEST(Alert, TakesItsLookFromTheProvidedTheme) {
    class Node* panel = nullptr;
    auto theme = GetDefaultLightTheme();
    theme.alert.rules.push_back({ .selector = { .intent = AlertIntent::Danger }, .style = { .borderColor = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, .padding = Vec4{ 30.0f, 30.0f, 30.0f, 30.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Alert({ .intent = AlertIntent::Danger, .title = "Error", .nodeProps = { .ref = &panel } });
        });
    });

    interface.render();

    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(panel->getBorder()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
    EXPECT_EQ(panel->getPaddingTop(), NodeValue(30.0f));
}
