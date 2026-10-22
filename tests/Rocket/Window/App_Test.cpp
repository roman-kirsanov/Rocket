/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <algorithm>
#include <Rocket/Window/Input.hpp>
#include <SDL3/SDL.h>
#include <Rocket/Base/PubSub.hpp>
#include <Rocket/Window/App.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(App, RunLoopEventsTimersAndExecPaths) {
    auto readyCount = 0;
    auto updateCount = 0;
    auto exitCount = 0;

    auto sub = Sub(App::OnEvent, [&](AppEvent const& event) {
        if (event.is<ReadyAppEvent>()) {
            readyCount += 1;
            /* Exit 250ms into the loop, so the time the platform takes to start does not count. */
            App::SetTimeout([] { App::Exit(); }, 250.0);
        } else if (event.is<UpdateAppEvent>()) {
            updateCount += 1;
        } else if (event.is<ExitAppEvent>()) {
            exitCount += 1;
        }
    });

    /* Timeouts fire once after their delay; cancelled timeouts never fire. */
    auto timeoutFired = 0;
    auto cancelledFired = 0;
    auto intervalFired = 0;

    App::SetTimeout([&] { timeoutFired += 1; }, 50.0);

    auto cancel = App::SetTimeout([&] { cancelledFired += 1; }, 50.0);
    cancel();

    auto stopInterval = App::SetInterval([&] { intervalFired += 1; }, 30.0);

    /* Exec paths are queryable. */
    ASSERT_TRUE(App::GetExecPath().empty() == false);
    ASSERT_TRUE(App::GetExecDir().empty() == false);

    App::Run(0, nullptr);

    /* One ready event before the loop, one exit event after it. */
    ASSERT_TRUE(readyCount == 1);
    ASSERT_TRUE(exitCount == 1);

    /* The loop ticked and ran the scheduled work. */
    ASSERT_TRUE(updateCount > 0);
    ASSERT_TRUE(timeoutFired == 1);
    ASSERT_TRUE(cancelledFired == 0);
    ASSERT_TRUE(intervalFired >= 2); /* ~250ms at a 30ms interval */

    stopInterval();
}

/* A gamepad plugged in while the app runs is announced, and so is its removal. */
TEST(App, PublishesGamepadConnectAndDisconnect) {
    auto id = SDL_JoystickID(0);
    auto events = std::vector<std::string>();
    auto sub = Sub(App::OnEvent, [&](AppEvent const& event) {
        if (event.is<ReadyAppEvent>()) {
            auto desc = SDL_VirtualJoystickDesc();
            SDL_INIT_INTERFACE(&desc);
            desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
            desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
            desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
            id = SDL_AttachVirtualJoystick(&desc);
            App::SetTimeout([] { App::Exit(); }, 5000.0);
        } else if (auto connect = event.as<GamepadConnectAppEvent>(); connect != nullptr && connect->getGamepad() == id) {
            events.push_back("connect");
            SDL_DetachVirtualJoystick(id);
        } else if (auto disconnect = event.as<GamepadDisconnectAppEvent>(); disconnect != nullptr && disconnect->getGamepad() == id) {
            events.push_back("disconnect");
            App::Exit();
        }
    });

    App::Run(0, nullptr);

    EXPECT_NE(id, SDL_JoystickID(0));
    EXPECT_EQ(events, std::vector<std::string>({ "connect", "disconnect" }));
}

/* A connected gamepad is listed with its name until it is removed. */
TEST(App, ListsConnectedGamepadsWithTheirNames) {
    auto id = SDL_JoystickID(0);
    auto listed = false;
    auto name = std::string();
    auto listedAfterRemoval = true;
    auto sub = Sub(App::OnEvent, [&](AppEvent const& event) {
        if (event.is<ReadyAppEvent>()) {
            auto desc = SDL_VirtualJoystickDesc();
            SDL_INIT_INTERFACE(&desc);
            desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
            desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
            desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
            desc.name = "Rocket Test Pad";
            id = SDL_AttachVirtualJoystick(&desc);
            App::SetTimeout([] { App::Exit(); }, 5000.0);
        } else if (auto connect = event.as<GamepadConnectAppEvent>(); connect != nullptr && connect->getGamepad() == id) {
            listed = std::ranges::contains(GetGamepads(), id);
            name = GetGamepadName(id);
            SDL_DetachVirtualJoystick(id);
        } else if (auto disconnect = event.as<GamepadDisconnectAppEvent>(); disconnect != nullptr && disconnect->getGamepad() == id) {
            listedAfterRemoval = std::ranges::contains(GetGamepads(), id);
            App::Exit();
        }
    });

    App::Run(0, nullptr);

    EXPECT_TRUE(listed);
    EXPECT_EQ(name, "Rocket Test Pad");
    EXPECT_FALSE(listedAfterRemoval);
    EXPECT_EQ(GetGamepadName(id), "");
}
