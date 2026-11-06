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
#include <Rocket/Window/WindowEvent.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Input.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static void _Type(_Interface& interface, std::string const& text) {
    for (auto const character : text) {
        interface.window.onEvent.publish(KeyDownWindowEvent(interface.window, Scancode::Unknown, "", KeyModifiers{}));
        interface.window.onEvent.publish(InputWindowEvent(interface.window, std::string(1, character)));
        interface.window.onEvent.publish(KeyUpWindowEvent(interface.window, Scancode::Unknown, "", KeyModifiers{}));
        interface.render();
    }
}

static void _PressEnter(_Interface& interface) {
    interface.window.onEvent.publish(KeyDownWindowEvent(interface.window, Scancode::Enter, std::string(Keycode::Enter), KeyModifiers{}));
    interface.window.onEvent.publish(KeyUpWindowEvent(interface.window, Scancode::Enter, std::string(Keycode::Enter), KeyModifiers{}));
    interface.render();
}

TEST(Input, ShowsItsValueOrElseItsPlaceholder) {
    class Node* empty = nullptr;
    class Node* filled = nullptr;
    auto interface = _Interface([&]{
        Input({ .placeholder = "Email", .nodeProps = { .ref = &empty } });
        Input({ .value = "me@example.com", .placeholder = "Email", .nodeProps = { .ref = &filled } });
    });

    interface.render();

    ASSERT_NE(empty, nullptr);
    ASSERT_NE(filled, nullptr);
    EXPECT_EQ(_FindTexts(empty), (std::vector<std::string>{ "", "Email" }));
    EXPECT_EQ(_FindTexts(filled), std::vector<std::string>{ "me@example.com" });
}

TEST(Input, PassesTheWholeTextOfEachEditAndHidesThePlaceholder) {
    class Node* root = nullptr;
    class Node* field = nullptr;
    auto value = std::string();
    auto changes = std::vector<std::string>();
    auto interface = _Interface([&]{
        Input({
            .value = value,
            .placeholder = "Name",
            .onChange = [&](std::string const& text) {
                changes.push_back(text);
                value = text;
            },
            .fieldProps = { .ref = &field },
            .nodeProps = { .ref = &root }
        });
    });
    interface.render();
    interface.document.focusNode(field);

    _Type(interface, "ab");
    interface.render();

    EXPECT_EQ(changes, (std::vector<std::string>{ "a", "ab" }));
    EXPECT_EQ(_FindTexts(root), std::vector<std::string>{ "ab" });
}

TEST(Input, CallsOnEnterWhenEnterIsPressed) {
    class Node* field = nullptr;
    auto enters = 0;
    auto interface = _Interface([&]{
        Input({ .onEnter = [&]{ enters += 1; }, .fieldProps = { .ref = &field } });
    });
    interface.render();
    interface.document.focusNode(field);

    _PressEnter(interface);

    EXPECT_EQ(enters, 1);
}

TEST(Input, StopsTypingAtItsMaxLength) {
    class Node* field = nullptr;
    auto value = std::string();
    auto interface = _Interface([&]{
        Input({ .value = value, .maxLength = 3, .onChange = [&](std::string const& text) { value = text; }, .fieldProps = { .ref = &field } });
    });
    interface.render();
    interface.document.focusNode(field);

    _Type(interface, "abcde");

    EXPECT_EQ(value, "abc");
}

TEST(Input, FocusesItsTextOnAPressAnywhereOnIt) {
    class Node* field = nullptr;
    class Node* prefix = nullptr;
    auto interface = _Interface([&]{
        Input({
            .prefix = [&]{
                Node({ .ref = &prefix, .width = 20.0f, .height = 20.0f });
            },
            .fieldProps = { .ref = &field }
        });
    });
    interface.render();

    auto const& rect = prefix->getComputedBorderRect();
    auto const center = Vec2{ rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f };
    interface.window.onEvent.publish(MouseMoveWindowEvent(interface.window, center, KeyModifiers{}));
    interface.window.onEvent.publish(MouseDownWindowEvent(interface.window, Mouse::LeftButton, center, KeyModifiers{}));
    interface.window.onEvent.publish(MouseUpWindowEvent(interface.window, Mouse::LeftButton, center, KeyModifiers{}));
    interface.render();

    ASSERT_NE(field, nullptr);
    EXPECT_TRUE(field->isFocused());
}

TEST(Input, TakesTheFocusWhenAskedTo) {
    class Node* field = nullptr;
    auto interface = _Interface([&]{
        Input({ .focus = true, .fieldProps = { .ref = &field } });
    });

    interface.render();
    interface.render();
    interface.render();

    ASSERT_NE(field, nullptr);
    EXPECT_TRUE(field->isFocused());
}

TEST(Input, HidesAPasswordAndCannotBeEditedWhenDisabled) {
    class Node* password = nullptr;
    class Node* disabled = nullptr;
    class Node* disabledRoot = nullptr;
    auto interface = _Interface([&]{
        Input({ .password = true, .fieldProps = { .ref = &password } });
        Input({ .disabled = true, .fieldProps = { .ref = &disabled }, .nodeProps = { .ref = &disabledRoot } });
    });

    interface.render();

    ASSERT_NE(password, nullptr);
    ASSERT_NE(disabled, nullptr);
    EXPECT_TRUE(password->getContentSecure());
    EXPECT_TRUE(password->getContentEditable());
    EXPECT_FALSE(disabled->getContentEditable());
    EXPECT_FALSE(disabledRoot->getMouseEvents());
}

TEST(Input, TakesItsLookFromTheProvidedThemeInEachState) {
    class Node* root = nullptr;
    class Node* field = nullptr;
    auto theme = GetDefaultLightTheme();
    theme.input.rules.push_back({ .selector = { .intent = InputIntent::Danger }, .style = { .borderColor = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, .minHeight = 50.0f } });
    theme.input.rules.push_back({ .selector = { .state = InputTheme::State::Focus }, .style = { .borderColor = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } }, .important = true });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Input({ .intent = InputIntent::Danger, .fieldProps = { .ref = &field }, .nodeProps = { .ref = &root } });
        });
    });
    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getBorder()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
    EXPECT_EQ(root->getMinHeight(), NodeValue(50.0f));

    interface.document.focusNode(field);
    interface.render();
    interface.render();

    EXPECT_EQ(root->getBorder()->as<ColorBrush>()->color, (Vec4{ 0.0f, 1.0f, 0.0f, 1.0f }));
}

TEST(Input, DrawsWithTheMaterialOfAMaterialTheme) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Input({ .nodeProps = { .ref = &root } });
        });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(root->getBackground()->as<ImageBrush>(), nullptr);
    EXPECT_NE(root->getBackground()->as<ImageBrush>()->image, nullptr);
}
