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
    ASSERT_TRUE(GetScancodeName(Scancode::Unknown) == "Unknown");
    ASSERT_TRUE(GetScancodeName(Scancode::Digit0) == "0");
    ASSERT_TRUE(GetScancodeName(Scancode::Digit9) == "9");
    ASSERT_TRUE(GetScancodeName(Scancode::KeyA) == "A");
    ASSERT_TRUE(GetScancodeName(Scancode::KeyZ) == "Z");
}

/* Different keys yield different names. */
TEST(Input, DifferentKeysYieldDifferentNames) {
    ASSERT_TRUE(GetScancodeName(Scancode::KeyA) != GetScancodeName(Scancode::KeyB));
    ASSERT_TRUE(GetScancodeName(Scancode::Digit0) != GetScancodeName(Scancode::Digit1));
}

/* An out-of-range key value falls back to "Unknown". */
TEST(Input, OutOfRangeKeyFallsBackToUnknown) {
    ASSERT_TRUE(GetScancodeName(static_cast<Scancode>(-1)) == "Unknown");
    ASSERT_TRUE(GetScancodeName(static_cast<Scancode>(100000)) == "Unknown");
}

/* Repeated lookups return a stable reference. */
TEST(Input, RepeatedLookupsReturnStableReference) {
    auto const& first = GetScancodeName(Scancode::KeyA);
    auto const& second = GetScancodeName(Scancode::KeyA);
    ASSERT_TRUE(&first == &second);
}

/* The US-layout keycode of a scancode: the lowercase character for
   character keys (numpad keys give their character, numpad Enter gives
   "Enter"), the name for the others, "Unidentified" for unknown keys. */
TEST(Input, DefaultKeycode) {
    ASSERT_TRUE(GetDefaultKeycode(Scancode::KeyA) == Keycode::A);
    ASSERT_TRUE(GetDefaultKeycode(Scancode::KeyZ) == "z");
    ASSERT_TRUE(GetDefaultKeycode(Scancode::Digit1) == "1");
    ASSERT_TRUE(GetDefaultKeycode(Scancode::Numpad1) == "1");
    ASSERT_TRUE(GetDefaultKeycode(Scancode::Space) == " ");
    ASSERT_TRUE(GetDefaultKeycode(Scancode::Backslash) == "\\");
    ASSERT_TRUE(GetDefaultKeycode(Scancode::Enter) == Keycode::Enter);
    ASSERT_TRUE(GetDefaultKeycode(Scancode::NumpadEnter) == Keycode::Enter);
    ASSERT_TRUE(GetDefaultKeycode(Scancode::ShiftRight) == Keycode::Shift);
    ASSERT_TRUE(GetDefaultKeycode(Scancode::F12) == "F12");
    ASSERT_TRUE(GetDefaultKeycode(Scancode::Unknown) == Keycode::Unknown);
}

/* Shortcut keycodes follow what the OS does for its shortcuts: the keycode,
   except digits by position on the digit row (AZERTY) and Latin letters by
   position for keys that type non-Latin letters (Russian, Greek). */
TEST(Input, ShortcutKeycode) {
    ASSERT_TRUE(GetShortcutKeycode(Scancode::KeyZ, "z") == "z");                  /* US */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::KeyZ, "w") == "w");                  /* AZERTY: the Z position is labelled W */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::KeyW, "z") == "z");                  /* AZERTY: the W position is labelled Z */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::Digit1, "&") == "1");                /* AZERTY digit row */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::KeyZ, "я") == "z");                  /* Russian */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::KeyC, "с") == "c");                  /* Russian: Cyrillic es */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::KeyY, "z") == "z");                  /* German QWERTZ */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::Semicolon, "ö") == "ö");             /* German: not a letter position, kept */
    ASSERT_TRUE(GetShortcutKeycode(Scancode::ArrowLeft, "ArrowLeft") == "ArrowLeft");
    ASSERT_TRUE(GetShortcutKeycode(Scancode::Equal, "=") == "=");
}
