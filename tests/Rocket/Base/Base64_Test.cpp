/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <Rocket/Base/Base64.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

TEST(EncodeBase64, EncodesNothingAsNothing) {
    EXPECT_EQ(EncodeBase64({}), "");
}

TEST(EncodeBase64, PadsWhatDoesNotFillTheLastBlock) {
    EXPECT_EQ(EncodeBase64({ 'M' }), "TQ==");
    EXPECT_EQ(EncodeBase64({ 'M', 'a' }), "TWE=");
    EXPECT_EQ(EncodeBase64({ 'M', 'a', 'n' }), "TWFu");
}

TEST(EncodeBase64, UsesTheLastTwoLettersOfTheAlphabet) {
    EXPECT_EQ(EncodeBase64({ 0xFB, 0xFF }), "+/8=");
}

TEST(DecodeBase64, DecodesWhatItEncodes) {
    auto const bytes = std::vector<std::uint8_t>{ 0x00, 0x10, 0x83, 0xFF, 0x7E };

    EXPECT_EQ(DecodeBase64(EncodeBase64(bytes)), bytes);
}

TEST(DecodeBase64, DecodesTextWithoutPadding) {
    EXPECT_EQ(DecodeBase64("TWE"), (std::vector<std::uint8_t>{ 'M', 'a' }));
}

TEST(DecodeBase64, RefusesACharacterThatIsNotBase64AndSaysWhich) {
    auto message = std::string();

    try {
        DecodeBase64("TW*u");
    } catch (std::exception const& e) {
        message = e.what();
    }

    EXPECT_EQ(message, "Failed to decode base64: Character is not base64: *");
}
