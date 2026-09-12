/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <Rocket/Window/Input.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* Known keys map to their expected names. */
TEST(Input, KnownKeysMapToExpectedNames) {
    ASSERT_TRUE(GetKeyName(Key::Unknown) == "Unknown");
    ASSERT_TRUE(GetKeyName(Key::Digit0) == "0");
    ASSERT_TRUE(GetKeyName(Key::Digit9) == "9");
    ASSERT_TRUE(GetKeyName(Key::KeyA) == "A");
    ASSERT_TRUE(GetKeyName(Key::KeyZ) == "Z");
}

/* Different keys yield different names. */
TEST(Input, DifferentKeysYieldDifferentNames) {
    ASSERT_TRUE(GetKeyName(Key::KeyA) != GetKeyName(Key::KeyB));
    ASSERT_TRUE(GetKeyName(Key::Digit0) != GetKeyName(Key::Digit1));
}

/* An out-of-range key value falls back to "Unknown". */
TEST(Input, OutOfRangeKeyFallsBackToUnknown) {
    ASSERT_TRUE(GetKeyName(static_cast<Key>(-1)) == "Unknown");
    ASSERT_TRUE(GetKeyName(static_cast<Key>(100000)) == "Unknown");
}

/* Repeated lookups return a stable reference. */
TEST(Input, RepeatedLookupsReturnStableReference) {
    auto const& first = GetKeyName(Key::KeyA);
    auto const& second = GetKeyName(Key::KeyA);
    ASSERT_TRUE(&first == &second);
}
