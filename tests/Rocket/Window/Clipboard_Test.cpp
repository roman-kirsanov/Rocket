/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <Rocket/Window/Clipboard.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(Clipboard, StringRoundTripsPreservingOriginal) {
    /* Preserve whatever text the user currently has on the clipboard. */
    auto original = GetClipboardString();

    /* A stored string is read back verbatim. */
    {
        SetClipboardString("rocket-clipboard-test");
        ASSERT_TRUE(GetClipboardString() == "rocket-clipboard-test");
    }

    /* Overwriting replaces the previous contents. */
    {
        SetClipboardString("second-value");
        ASSERT_TRUE(GetClipboardString() == "second-value");
    }

    /* Unicode text survives the round-trip. */
    {
        SetClipboardString("\xC3\xA9\xE2\x82\xAC"); /* é€ */
        ASSERT_TRUE(GetClipboardString() == "\xC3\xA9\xE2\x82\xAC");
    }

    /* Restore the user's clipboard. */
    SetClipboardString(original);
    ASSERT_TRUE(GetClipboardString() == original);
}
