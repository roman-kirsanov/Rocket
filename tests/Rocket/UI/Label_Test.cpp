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
#include <Rocket/Window/WindowEvent.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Label.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Label, ShowsItsTextThenTheMarkOfARequiredField) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Label({ .text = "Email", .required = true, .nodeProps = { .ref = &root } });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(_FindTexts(root), (std::vector<std::string>{ "Email", "*" }));
}

TEST(Label, ShowsAnIconOnlyWhenItHasATooltip) {
    class Node* plain = nullptr;
    class Node* icon = nullptr;
    auto interface = _Interface([&]{
        Label({ .text = "Name", .nodeProps = { .ref = &plain } });
        Label({ .text = "Email", .tooltip = "We never share it.", .iconProps = { .nodeProps = { .ref = &icon } } });
    });

    interface.render();

    ASSERT_NE(plain, nullptr);
    ASSERT_NE(icon, nullptr);
    EXPECT_EQ(plain->getFirstChild()->getNextSibling(), nullptr);
    EXPECT_NE(icon->getBackground()->as<ImageBrush>(), nullptr);
}

TEST(Label, ShowsItsTooltipAndBrightensItsIconWhenThePointerIsOnTheIcon) {
    class Node* root = nullptr;
    class Node* icon = nullptr;
    auto interface = _Interface([&]{
        Label({ .text = "Email", .tooltip = "We never share it.", .iconProps = { .nodeProps = { .ref = &icon } }, .tooltipProps = { .delayShow = 0.0 }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    ASSERT_NE(icon, nullptr);
    EXPECT_EQ(icon->getOpacity(), 0.5f);

    auto const& rect = icon->getComputedBorderRect();
    interface.window.onEvent.publish(MouseMoveWindowEvent(interface.window, { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f }, KeyModifiers{}));
    interface.render();
    interface.render();
    interface.render();

    EXPECT_EQ(icon->getOpacity(), 1.0f);
    EXPECT_EQ(_FindTexts(root), (std::vector<std::string>{ "Email", "We never share it." }));
}

TEST(Label, TakesItsLookFromTheProvidedTheme) {
    class Node* root = nullptr;
    class Node* mark = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.label.rules.push_back({ .style = { .fontSize = 18.0f, .requiredMark = "required", .requiredColor = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Label({ .text = "Email", .required = true, .requiredProps = { .ref = &mark }, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(mark, nullptr);
    EXPECT_EQ(root->getFontSize(), 18.0f);
    EXPECT_EQ(mark->getContent(), "required");
    EXPECT_EQ(mark->getTextColor(), (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
}
