/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <tuple>
#include <string>
#include <vector>
#include <algorithm>
#include <Rocket/Window/Window.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* Note: windows are never made visible in this test, so no window flashes
   on screen while the suite runs. */

/* Construction registers the window; destruction unregisters it. */
TEST(Window, ConstructionRegistersDestructionUnregisters) {
    auto countBefore = Window::GetWindows().size();

    {
        auto window = Window();
        auto& windows = Window::GetWindows();
        ASSERT_TRUE(windows.size() == countBefore + 1);
        ASSERT_TRUE(std::find(windows.begin(), windows.end(), &window) != windows.end());
    }

    ASSERT_TRUE(Window::GetWindows().size() == countBefore);
}

/* Properties round-trip through their setters and getters. */
TEST(Window, PropertiesRoundTripThroughSettersAndGetters) {
    auto window = Window();

    window.setTitle("Rocket Test Window");
    ASSERT_TRUE(window.getTitle() == "Rocket Test Window");

    window.setSize({ 640.0f, 480.0f });
    ASSERT_TRUE(window.getSize() == Vec2(640.0f, 480.0f));

    window.setPosition({ 100.0f, 120.0f });
    ASSERT_TRUE(window.getPosition() == Vec2(100.0f, 120.0f));

    window.setMaximizable(true);
    ASSERT_TRUE(window.getMaximizable() == true);

    window.setClosable(false);
    ASSERT_TRUE(window.getClosable() == false);
    window.setClosable(true);
    ASSERT_TRUE(window.getClosable() == true);

    window.setSizable(false);
    ASSERT_TRUE(window.getSizable() == false);

    window.setMaximizable(false);
    ASSERT_TRUE(window.getMaximizable() == false);

    window.setMinimizable(false);
    ASSERT_TRUE(window.getMinimizable() == false);

    window.setTopmost(true);
    ASSERT_TRUE(window.getTopmost() == true);
    window.setTopmost(false);
    ASSERT_TRUE(window.getTopmost() == false);
}

/* Cursor: defaults to Cursor::Default and round-trips through setCursor,
   even on a hidden window. */
TEST(Window, CursorDefaultsAndRoundTrips) {
    auto window = Window();
    ASSERT_TRUE(window.getCursor() == Cursor::Default);

    window.setCursor(Cursor::Pointer);
    ASSERT_TRUE(window.getCursor() == Cursor::Pointer);

    window.setCursor(Cursor::Default);
    ASSERT_TRUE(window.getCursor() == Cursor::Default);
}

/* A new window starts hidden, unfocused, and windowed. */
TEST(Window, NewWindowStartsHiddenUnfocusedAndWindowed) {
    auto window = Window();
    ASSERT_TRUE(window.getVisible() == false);
    ASSERT_TRUE(window.isMaximized() == false);
    ASSERT_TRUE(window.isMinimized() == false);
    ASSERT_TRUE(window.getHandle() != nullptr);
    ASSERT_TRUE(window.getScale() > 0.0f);
}

/* Multiple windows coexist in the registry. */
TEST(Window, MultipleWindowsCoexistInRegistry) {
    auto countBefore = Window::GetWindows().size();
    auto first = Window();
    auto second = Window();
    ASSERT_TRUE(Window::GetWindows().size() == countBefore + 2);
}

/* Showing and hiding publish exactly one Show/HideWindowEvent each. */
TEST(Window, ShowAndHidePublishOneEventEach) {
    auto window = Window();
    auto showCount = 0;
    auto hideCount = 0;

    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (event.is<ShowWindowEvent>()) {
            showCount += 1;
        } else if (event.is<HideWindowEvent>()) {
            hideCount += 1;
        }
    });

    window.setVisible(true);
    ASSERT_TRUE(window.getVisible() == true);
    ASSERT_TRUE(showCount == 1);
    ASSERT_TRUE(hideCount == 0);

    window.setVisible(false);
    ASSERT_TRUE(window.getVisible() == false);
    ASSERT_TRUE(showCount == 1);
    ASSERT_TRUE(hideCount == 1);
}

/* The input area starts unset and round-trips through setInputArea, including
   clearing it again. */
TEST(Window, InputAreaDefaultsAndRoundTrips) {
    auto window = Window();
    ASSERT_TRUE(window.getInputArea().has_value() == false);

    window.setInputArea(Vec4{ 10.0f, 20.0f, 2.0f, 16.0f });
    ASSERT_TRUE(window.getInputArea().has_value());
    ASSERT_TRUE(*window.getInputArea() == Vec4(10.0f, 20.0f, 2.0f, 16.0f));

    window.setInputArea(std::nullopt);
    ASSERT_TRUE(window.getInputArea().has_value() == false);
}

/* Input and composition events carry their text and, for composition, the
   caret and selected segment; both reach onEvent subscribers. */
TEST(Window, InputAndCompositionEventsCarryTheirPayloads) {
    auto window = Window();

    auto texts = std::vector<std::string>();
    auto compositions = std::vector<std::tuple<std::string, std::int32_t, std::int32_t>>();
    auto sub = Sub<WindowEvent const&>(window.onEvent, [&](WindowEvent const& event) {
        if (auto e = event.as<InputWindowEvent>()) texts.push_back(e->getText());
        if (auto e = event.as<CompositionWindowEvent>()) compositions.push_back({ e->getText(), e->getCursor(), e->getSelectionLength() });
    });

    window.onEvent.publish(CompositionWindowEvent(window, "にほんご", 2, 2));
    window.onEvent.publish(CompositionWindowEvent(window, "", 0, 0));
    window.onEvent.publish(InputWindowEvent(window, "日本語"));

    ASSERT_TRUE(compositions.size() == 2);
    ASSERT_TRUE(compositions.at(0) == std::make_tuple(std::string("にほんご"), 2, 2));
    ASSERT_TRUE(std::get<0>(compositions.at(1)).empty());
    ASSERT_TRUE(texts == std::vector<std::string>({ "日本語" }));
}
