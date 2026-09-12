/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Time.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* The current time is within a plausible wall-clock range. */
TEST(Time, CurrentTimeInPlausibleRange) {
    auto const time = GetTime();
    ASSERT_TRUE(time > 1577836800000LL); /* after 2020-01-01 */
    ASSERT_TRUE(time < 4102444800000LL); /* before 2100-01-01 */
}

/* Successive readings never go backwards. */
TEST(Time, SuccessiveReadingsMonotonic) {
    auto previous = GetTime();
    for (int i = 0; i < 1000; i++) {
        auto const current = GetTime();
        ASSERT_TRUE(previous <= current);
        previous = current;
    }
}
