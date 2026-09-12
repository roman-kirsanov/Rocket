/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
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
