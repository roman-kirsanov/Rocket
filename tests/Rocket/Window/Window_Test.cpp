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
#include <Rocket/Window/App.hpp>
#include <Rocket/Window/Window.hpp>
#include <gtest/gtest.h>

#ifndef __APPLE__
#include <SDL3/SDL.h>
#endif

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
}

/* The position round-trips through its setter and getter. */
TEST(Window, PositionRoundTripsThroughSetterAndGetter) {
#ifndef __APPLE__
    // TODO: Run on SDL3 once the video driver SDL chooses can place windows (Wayland cannot).
    GTEST_SKIP() << "depends on the video driver SDL chooses";
#endif

    auto window = Window();

    window.setPosition({ 100.0f, 120.0f });
    ASSERT_TRUE(window.getPosition() == Vec2(100.0f, 120.0f));
}

/* Topmost round-trips through its setter and getter. */
TEST(Window, TopmostRoundTripsThroughSetterAndGetter) {
#ifndef __APPLE__
    // TODO: Run on SDL3 once the video driver SDL chooses can keep windows on top (Wayland cannot).
    GTEST_SKIP() << "depends on the video driver SDL chooses";
#endif

    auto window = Window();

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

#ifndef __APPLE__

/* Pushes an SDL event for the window, as the platform would deliver it. */
static void _PushEvent(Window& window, SDL_Event event) {
    event.common.timestamp = SDL_GetTicksNS();
    SDL_PushEvent(&event);
}

static void _PushMotion(Window& window, float x, float y, SDL_MouseButtonFlags state = 0) {
    auto event = SDL_Event{};
    event.motion.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.state = state;
    event.motion.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(window.getHandle()));
    event.motion.x = x;
    event.motion.y = y;
    _PushEvent(window, event);
}

static void _PushWheel(Window& window, float x, float y) {
    auto event = SDL_Event{};
    event.wheel.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(window.getHandle()));
    event.wheel.x = x;
    event.wheel.y = y;
    _PushEvent(window, event);
}

static void _PushButton(Window& window, SDL_EventType type, float x, float y) {
    auto event = SDL_Event{};
    event.button.type = type;
    event.button.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(window.getHandle()));
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = (type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    event.button.clicks = 1;
    event.button.x = x;
    event.button.y = y;
    _PushEvent(window, event);
}

/* Runs the app loop for a moment, so it handles the pushed events. */
static void _RunBriefly() {
    App::SetTimeout([] { App::Exit(); }, 50.0);
    App::Run(0, nullptr);
}

/* Moves that arrive together reach the window as one, at the last position;
   a press between them keeps its place in the order. */
TEST(Window, MovesArrivingTogetherCoalesce) {
    auto window = Window();
    auto events = std::vector<std::string>();
    auto positions = std::vector<Vec2>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto move = event.as<MouseMoveWindowEvent>()) {
            events.push_back("move");
            positions.push_back(move->getPosition());
        } else if (event.is<MouseDownWindowEvent>()) {
            events.push_back("down");
        } else if (event.is<MouseUpWindowEvent>()) {
            events.push_back("up");
        }
    });

    for (auto i = 1; i <= 50; i++) {
        _PushMotion(window, static_cast<float>(i), 10.0f);
    }
    _PushButton(window, SDL_EVENT_MOUSE_BUTTON_DOWN, 50.0f, 10.0f);
    for (auto i = 51; i <= 100; i++) {
        _PushMotion(window, static_cast<float>(i), 10.0f, SDL_BUTTON_LMASK);
    }
    _PushButton(window, SDL_EVENT_MOUSE_BUTTON_UP, 100.0f, 10.0f);

    _RunBriefly();

    ASSERT_TRUE(events == std::vector<std::string>({ "move", "down", "move", "up" }));
    ASSERT_TRUE(positions.front() == Vec2(50.0f, 10.0f));
    ASSERT_TRUE(positions.back() == Vec2(100.0f, 10.0f));
}

/* Wheel turns that arrive together reach the window as one, their sum. */
TEST(Window, WheelTurnsArrivingTogetherSum) {
    auto window = Window();
    auto wheels = std::vector<Vec2>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto wheel = event.as<MouseWheelWindowEvent>()) {
            wheels.push_back(wheel->getPosition());
        }
    });

    _PushWheel(window, 0.0f, 2.0f);
    _RunBriefly();

    for (auto i = 0; i < 4; i++) {
        _PushWheel(window, 0.0f, 0.5f);
    }
    _RunBriefly();

    ASSERT_TRUE(wheels.size() == 2);
    ASSERT_TRUE(wheels.at(1) == wheels.at(0)); /* four half notches turn as far as two notches */
}

#endif
