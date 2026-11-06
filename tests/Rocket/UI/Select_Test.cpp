/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstddef>
#include <string>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <Rocket/Window/Input.hpp>
#include <Rocket/Window/WindowEvent.hpp>
#include <_Helpers/Interface.hpp>
#include <Rocket/UI/Select.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static auto const _fruits = std::vector<std::string>{ "Apple", "Banana", "Cherry", "Grape" };

static class Node* _FindOption(class Node* root, std::string const& label) {
    auto const texts = _FindNodes(root, [&](class Node const& node) {
        return (node.getContent() == label);
    });

    return (texts.empty() ? nullptr : texts.back()->getParent());
}

static void _Type(_Interface& interface, std::string const& text) {
    for (auto const character : text) {
        interface.window.onEvent.publish(KeyDownWindowEvent(interface.window, Scancode::Unknown, "", KeyModifiers{}));
        interface.window.onEvent.publish(InputWindowEvent(interface.window, std::string(1, character)));
        interface.window.onEvent.publish(KeyUpWindowEvent(interface.window, Scancode::Unknown, "", KeyModifiers{}));
        interface.render();
    }
}

TEST(Select, ShowsTheSelectedOptionOrElseThePlaceholder) {
    class Node* empty = nullptr;
    class Node* chosen = nullptr;
    auto interface = _Interface([&]{
        Select({ .options = _fruits, .placeholder = "Pick a fruit", .nodeProps = { .ref = &empty } });
        Select({ .options = _fruits, .selected = 2, .nodeProps = { .ref = &chosen } });
    });

    interface.render();

    ASSERT_NE(empty, nullptr);
    ASSERT_NE(chosen, nullptr);
    EXPECT_EQ(_FindTexts(empty), std::vector<std::string>{ "Pick a fruit" });
    EXPECT_EQ(_FindTexts(chosen), std::vector<std::string>{ "Cherry" });
}

TEST(Select, OpensItsListOnAClickAndPicksTheClickedOption) {
    class Node* root = nullptr;
    auto picks = std::vector<std::size_t>();
    auto interface = _Interface([&]{
        Select({ .options = _fruits, .onChange = [&](std::size_t index) { picks.push_back(index); }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);
    interface.render();

    EXPECT_EQ(_FindTexts(root), (std::vector<std::string>{ "", "Apple", "Banana", "Cherry", "Grape" }));

    _Click(_FindOption(root, "Banana"));
    interface.render();

    EXPECT_EQ(picks, std::vector<std::size_t>{ 1 });
    EXPECT_EQ(_FindTexts(root), std::vector<std::string>{ "" });
}

TEST(Select, IsDrivenByTheKeyboard) {
    class Node* root = nullptr;
    auto picks = std::vector<std::size_t>();
    auto interface = _Interface([&]{
        Select({ .options = _fruits, .selected = 1, .onChange = [&](std::size_t index) { picks.push_back(index); }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _PressKey(root, Scancode::ArrowDown, Keycode::ArrowDown);
    interface.render();
    _PressKey(root, Scancode::ArrowDown, Keycode::ArrowDown);
    _PressKey(root, Scancode::ArrowDown, Keycode::ArrowDown);
    _PressKey(root, Scancode::ArrowUp, Keycode::ArrowUp);
    _PressKey(root, Scancode::Enter, Keycode::Enter);
    interface.render();

    EXPECT_EQ(picks, std::vector<std::size_t>{ 2 });
    EXPECT_EQ(_FindOption(root, "Apple"), nullptr);

    _PressKey(root, Scancode::Space, Keycode::Space);
    interface.render();

    EXPECT_NE(_FindOption(root, "Apple"), nullptr);

    _PressKey(root, Scancode::Escape, Keycode::Escape);
    interface.render();

    EXPECT_EQ(_FindOption(root, "Apple"), nullptr);
    EXPECT_EQ(picks, std::vector<std::size_t>{ 2 });
}

TEST(Select, NarrowsTheOptionsToTheSearch) {
    class Node* root = nullptr;
    auto picks = std::vector<std::size_t>();
    auto interface = _Interface([&]{
        Select({ .options = _fruits, .searchable = true, .onChange = [&](std::size_t index) { picks.push_back(index); }, .nodeProps = { .ref = &root } });
    });
    interface.render();

    _Click(root);
    interface.render();
    interface.render();
    _Type(interface, "AP");

    EXPECT_NE(_FindOption(root, "Apple"), nullptr);
    EXPECT_NE(_FindOption(root, "Grape"), nullptr);
    EXPECT_EQ(_FindOption(root, "Banana"), nullptr);

    _PressKey(root, Scancode::ArrowDown, Keycode::ArrowDown);
    _PressKey(root, Scancode::ArrowDown, Keycode::ArrowDown);
    _PressKey(root, Scancode::Enter, Keycode::Enter);

    EXPECT_EQ(picks, std::vector<std::size_t>{ 3 });
}

TEST(Select, ClosesOnAPressOutside) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Select({ .options = _fruits, .nodeProps = { .ref = &root, .width = 200.0f } });
    });
    interface.render();
    _Click(root);
    interface.render();

    interface.window.onEvent.publish(MouseMoveWindowEvent(interface.window, { 600.0f, 400.0f }, KeyModifiers{}));
    interface.window.onEvent.publish(MouseDownWindowEvent(interface.window, Mouse::LeftButton, { 600.0f, 400.0f }, KeyModifiers{}));
    interface.window.onEvent.publish(MouseUpWindowEvent(interface.window, Mouse::LeftButton, { 600.0f, 400.0f }, KeyModifiers{}));
    interface.render();

    EXPECT_EQ(_FindOption(root, "Apple"), nullptr);
}

TEST(Select, IgnoresThePointerWhenDisabled) {
    class Node* root = nullptr;
    auto interface = _Interface([&]{
        Select({ .options = _fruits, .disabled = true, .nodeProps = { .ref = &root } });
    });

    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_FALSE(root->getMouseEvents());
    EXPECT_EQ(root->getTabIndex(), 0);
}

TEST(Select, TakesItsLookFromTheProvidedTheme) {
    class Node* root = nullptr;
    class Node* list = nullptr;
    auto theme = GetDefaultLightTheme();
    theme.select.rules.push_back({ .selector = { .intent = SelectIntent::Success }, .style = { .borderColor = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f }, .listBackground = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } } });
    auto interface = _Interface([&]{
        Context(theme, [&]{
            Select({ .options = _fruits, .intent = SelectIntent::Success, .listProps = { .ref = &list }, .nodeProps = { .ref = &root } });
        });
    });
    interface.render();

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getBorder()->as<ColorBrush>()->color, (Vec4{ 0.0f, 1.0f, 0.0f, 1.0f }));

    _Click(root);
    interface.render();

    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->getBackground()->as<ColorBrush>()->color, (Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }));
}

TEST(Select, DrawsItsFieldAndListWithTheMaterialOfAMaterialTheme) {
    class Node* root = nullptr;
    class Node* list = nullptr;
    auto interface = _Interface([&]{
        Context(GetPixelDarkTheme(), [&]{
            Select({ .options = _fruits, .listProps = { .ref = &list }, .nodeProps = { .ref = &root } });
        });
    });
    interface.render();
    _Click(root);
    interface.render();

    ASSERT_NE(root, nullptr);
    ASSERT_NE(list, nullptr);
    EXPECT_NE(root->getBackground()->as<ImageBrush>()->image, nullptr);
    EXPECT_NE(list->getBackground()->as<ImageBrush>()->image, nullptr);
}
