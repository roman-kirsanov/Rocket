/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
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

    /* Exit the run loop after 250ms. */
    App::SetTimeout([] { App::Exit(); }, 250.0);

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
