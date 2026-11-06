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
#include <Rocket/UI/Callout.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Callout, HoldsItsChildrenInItsContent) {
    class Node* root = nullptr;
    class Node* content = nullptr;
    auto interface = _Interface([&]{
        Callout({ .contentProps = { .ref = &content }, .nodeProps = { .ref = &root } }, [&]{
            Node({ .display = NodeDisplay::Text, .content = "Saved" });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(content, nullptr);
    EXPECT_EQ(root->getFirstChild(), content);
    EXPECT_EQ(_FindTexts(content), std::vector<std::string>{ "Saved" });
}

TEST(Callout, ShowsTheThemesIconOnlyWhenAskedTo) {
    class Node* plain = nullptr;
    class Node* withIcon = nullptr;
    class Node* icon = nullptr;
    auto interface = _Interface([&]{
        Callout({ .nodeProps = { .ref = &plain } });
        Callout({ .intent = CalloutIntent::Danger, .showIcon = true, .iconProps = { .nodeProps = { .ref = &icon } }, .nodeProps = { .ref = &withIcon } });
    });

    interface.render();

    ASSERT_NE(plain, nullptr);
    ASSERT_NE(withIcon, nullptr);
    ASSERT_NE(icon, nullptr);
    EXPECT_EQ(plain->getFirstChild()->getNextSibling(), nullptr);
    EXPECT_EQ(withIcon->getFirstChild(), icon);
    EXPECT_NE(icon->getBackground()->as<ImageBrush>(), nullptr);
}

TEST(Callout, TakesItsLookFromTheProvidedTheme) {
    class Node* root = nullptr;
    auto theme = GetDefaultLightTheme();
    theme.callout.rules.push_back({ .selector = { .intent = CalloutIntent::Warning }, .style = { .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, .padding = Vec4{ 20.0f, 20.0f, 20.0f, 20.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Callout({ .intent = CalloutIntent::Warning, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getBackground()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
    EXPECT_EQ(root->getPaddingLeft(), NodeValue(20.0f));
}

TEST(Callout, DrawsWithTheMaterialOfAMaterialTheme) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Callout({ .intent = CalloutIntent::Primary, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(root->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(root->getBackground()->as<ImageBrush>()->image, nullptr);
}

TEST(Callout, LetsTheCallerOverrideTheNode) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Callout({ .nodeProps = { .ref = &root, .padding = 4.0f } });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getPaddingLeft(), NodeValue(4.0f));
    EXPECT_EQ(root->getPaddingBottom(), NodeValue(4.0f));
}
