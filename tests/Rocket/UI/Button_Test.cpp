/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Button.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Button, ShowsItsLabel) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Button({ .label = "Add", .nodeProps = { .ref = &root } });
    });

    interface.render();

    EXPECT_EQ(_FindTexts(root), std::vector<std::string>{ "Add" });
}

TEST(Button, CallsItsHandlerWhenClicked) {
    class Node* root = nullptr;
    auto clicks = 0;
    auto interface = _Interface([&]{
        Button({ .label = "Add", .onClick = [&]{ clicks += 1; }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);

    EXPECT_EQ(clicks, 1);
}

TEST(Button, DoesNotCallItsHandlerWhenDisabled) {
    class Node* root = nullptr;
    auto clicks = 0;
    auto interface = _Interface([&]{
        Button({ .label = "Add", .disabled = true, .onClick = [&]{ clicks += 1; }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);

    EXPECT_EQ(clicks, 0);
}

TEST(Button, InvitesAClickOnlyWhenEnabled) {
    class Node* enabled = nullptr;
    class Node* disabled = nullptr;
    auto interface = _Interface([&]{
        Button({ .label = "Add", .nodeProps = { .ref = &enabled } });
        Button({ .label = "Remove", .disabled = true, .nodeProps = { .ref = &disabled } });
    });

    interface.render();

    EXPECT_EQ(_FindClickableNodes(enabled).size(), 1);
    EXPECT_EQ(_FindClickableNodes(disabled).size(), 0);
}

TEST(Button, ShowsItsIconBeforeItsLabel) {
    class Node* root = nullptr;
    auto const image = Image(Vec2(1.0f, 1.0f));
    auto interface = _Interface([&]{
        Button({ .label = "Add", .icon = ImageIconData{ image, ImageFilter::Nearest }, .nodeProps = { .ref = &root } });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(root->getFirstChild(), nullptr);
    ASSERT_TRUE(root->getFirstChild()->getBackground().has_value());
    ASSERT_NE(root->getFirstChild()->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_EQ(root->getFirstChild()->getBackground()->as<ImageBrush>()->image, &image);
    EXPECT_EQ(_FindTexts(root), std::vector<std::string>{ "Add" });
}

TEST(Button, ColorsItsIconByTheIntentGivenForTheIcon) {
    class Node* success = nullptr;
    class Node* danger = nullptr;
    auto const svg = std::string(R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M4 4h16v16h-16z"/></svg>)");
    auto interface = _Interface([&]{
        Button({ .icon = SvgIconData{ svg }, .iconProps = { .intent = IconIntent::Success }, .nodeProps = { .ref = &success } });
        Button({ .icon = SvgIconData{ svg }, .iconProps = { .intent = IconIntent::Danger }, .nodeProps = { .ref = &danger } });
    });

    interface.render();

    ASSERT_NE(success, nullptr);
    ASSERT_NE(danger, nullptr);
    ASSERT_NE(success->getFirstChild(), nullptr);
    ASSERT_NE(danger->getFirstChild(), nullptr);
    EXPECT_NE(
        success->getFirstChild()->getBackground()->as<ImageBrush>()->image,
        danger->getFirstChild()->getBackground()->as<ImageBrush>()->image
    );
}

TEST(Button, ColorsItsIconItselfWhenNoIntentIsGivenForTheIcon) {
    class Node* first = nullptr;
    class Node* second = nullptr;
    auto const svg = std::string(R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M4 4h16v16h-16z"/></svg>)");
    auto interface = _Interface([&]{
        Button({ .icon = SvgIconData{ svg }, .nodeProps = { .ref = &first } });
        Button({ .icon = SvgIconData{ svg }, .nodeProps = { .ref = &second } });
    });

    interface.render();

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_NE(first->getFirstChild(), nullptr);
    ASSERT_NE(second->getFirstChild(), nullptr);
    EXPECT_EQ(
        first->getFirstChild()->getBackground()->as<ImageBrush>()->image,
        second->getFirstChild()->getBackground()->as<ImageBrush>()->image
    );
}

TEST(Button, TakesThePaddingItIsGivenForEveryEdge) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{ Button({ .label = "Save", .nodeProps = { .ref = &root, .padding = 2.0f } }); });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getPaddingLeft(), NodeValue(2.0f));
    EXPECT_EQ(root->getPaddingTop(), NodeValue(2.0f));
    EXPECT_EQ(root->getPaddingRight(), NodeValue(2.0f));
    EXPECT_EQ(root->getPaddingBottom(), NodeValue(2.0f));
}

TEST(Button, TakesItsLookFromTheProvidedTheme) {
    class Node* root = nullptr;
    auto theme = GetDefaultDarkTheme();
    theme.button.rules.push_back({ .selector = { .state = ButtonTheme::State::Disabled }, .style = { .opacity = 0.3f } });
    theme.button.rules.push_back({ .style = { .minHeight = 50.0f } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Button({ .label = "Add", .disabled = true, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getOpacity(), 0.3f);
    EXPECT_EQ(root->getMinHeight(), NodeValue(50.0f));
}

static Theme _CreateStandardTheme() {
    auto theme = GetDefaultDarkTheme();

    theme.button.rules = {
        { .style = { .background = Vec4{ 0.1f, 0.2f, 0.3f, 1.0f }, .borderColor = Vec4{ 0.4f, 0.5f, 0.6f, 1.0f }, .borderWidth = 1.0f, .borderRadius = 4.0f, .color = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f } } },
        { .selector = { .state = ButtonTheme::State::Hover }, .style = { .background = Vec4{ 0.2f, 0.3f, 0.4f, 1.0f } } },
        { .selector = { .state = ButtonTheme::State::Active }, .style = { .background = Vec4{ 0.0f, 0.1f, 0.2f, 1.0f } } }
    };

    return theme;
}

static Vec4 _GetBackground(class Node* node) {
    return node->getBackground()->as<ColorBrush>()->color;
}

TEST(Button, LetsTheMoreSpecificRuleWin) {
    class Node* root = nullptr;
    auto theme = _CreateStandardTheme();
    theme.button.rules.push_back({ .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger }, .style = { .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } });
    theme.button.rules.push_back({ .selector = { .intent = ButtonIntent::Danger }, .style = { .background = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Button({ .label = "Delete", .intent = ButtonIntent::Danger, .kind = ButtonKind::Outlined, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(_GetBackground(root), (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
}

TEST(Button, LetsTheLaterRuleWinBetweenEquallySpecificOnes) {
    class Node* root = nullptr;
    auto theme = _CreateStandardTheme();
    theme.button.rules.push_back({ .selector = { .intent = ButtonIntent::Danger }, .style = { .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } });
    theme.button.rules.push_back({ .selector = { .kind = ButtonKind::Outlined }, .style = { .background = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Button({ .label = "Delete", .intent = ButtonIntent::Danger, .kind = ButtonKind::Outlined, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(_GetBackground(root), (Vec4{ 0.0f, 1.0f, 0.0f, 1.0f }));
}

TEST(Button, LetsAnImportantRuleBeatAMoreSpecificOne) {
    class Node* root = nullptr;
    auto theme = _CreateStandardTheme();
    theme.button.rules.push_back({ .style = { .background = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } }, .important = true });
    theme.button.rules.push_back({ .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger }, .style = { .background = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Button({ .label = "Delete", .intent = ButtonIntent::Danger, .kind = ButtonKind::Outlined, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(_GetBackground(root), (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
}

TEST(Button, CascadesOnlyTheFieldsARuleSets) {
    class Node* root = nullptr;
    auto theme = _CreateStandardTheme();
    theme.button.rules.push_back({ .selector = { .intent = ButtonIntent::Danger }, .style = { .borderRadius = 0.0f } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Button({ .label = "Delete", .intent = ButtonIntent::Danger, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getBorderTopLeftRadius(), 0.0f);
    EXPECT_EQ(root->getBorderLeftWidth(), 1.0f);
    EXPECT_EQ(_GetBackground(root), (Vec4{ 0.1f, 0.2f, 0.3f, 1.0f }));
}

TEST(Button, DrawsAStandardAppearanceWithPlainColors) {
    class Node* root = nullptr;
    auto const theme = _CreateStandardTheme();
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Button({ .label = "Add", .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->getBackground().has_value());
    ASSERT_NE(root->getBackground()->as<ColorBrush>(), nullptr);
    EXPECT_EQ(root->getBackground()->as<ColorBrush>()->color, (Vec4{ 0.1f, 0.2f, 0.3f, 1.0f }));
    ASSERT_TRUE(root->getBorder().has_value());
    EXPECT_EQ(root->getBorder()->as<ColorBrush>()->color, (Vec4{ 0.4f, 0.5f, 0.6f, 1.0f }));
    EXPECT_EQ(root->getBorderLeftWidth(), 1.0f);
    EXPECT_EQ(root->getBorderTopLeftRadius(), 4.0f);
}

TEST(Button, SquaresTheCornersOfALeftOutEdgeInAStandardAppearance) {
    class Node* root = nullptr;
    auto const theme = _CreateStandardTheme();
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Button({ .label = "Add", .noLeftBorder = true, .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getBorderLeftWidth(), 0.0f);
    EXPECT_EQ(root->getBorderRightWidth(), 1.0f);
    EXPECT_EQ(root->getBorderTopLeftRadius(), 0.0f);
    EXPECT_EQ(root->getBorderBottomLeftRadius(), 0.0f);
    EXPECT_EQ(root->getBorderTopRightRadius(), 4.0f);
}

TEST(Button, DrawsAMaterialAppearanceWithTheThemesMaterial) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Button({ .label = "Add", .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->getBackground().has_value());
    ASSERT_NE(root->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(root->getBackground()->as<ImageBrush>()->image, nullptr);
}
