/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <Rocket/Base/String.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* Decoding an ASCII string yields one codepoint per character. */
TEST(String, AsciiDecodesOneCodepointPerCharacter) {
    std::vector<std::uint32_t> codepoints;
    StringToCodepoints("abc", codepoints);
    ASSERT_TRUE(codepoints.size() == 3);
    ASSERT_TRUE(codepoints[0] == 97);
    ASSERT_TRUE(codepoints[1] == 98);
    ASSERT_TRUE(codepoints[2] == 99);
}

/* Decoding an empty string yields no codepoints. */
TEST(String, EmptyStringDecodesToNoCodepoints) {
    std::vector<std::uint32_t> codepoints;
    StringToCodepoints("", codepoints);
    ASSERT_TRUE(codepoints.empty());
}

/* Decoding clears any previous contents of the output vector. */
TEST(String, DecodingClearsOutputVector) {
    std::vector<std::uint32_t> codepoints = { 1, 2, 3 };
    StringToCodepoints("x", codepoints);
    ASSERT_TRUE(codepoints.size() == 1);
    ASSERT_TRUE(codepoints[0] == 120);
}

/* Multi-byte UTF-8 sequences decode to the correct codepoints. */
TEST(String, MultiByteSequencesDecode) {
    std::vector<std::uint32_t> codepoints;
    StringToCodepoints("\xC3\xA9", codepoints); /* é U+00E9, 2 bytes */
    ASSERT_TRUE(codepoints.size() == 1);
    ASSERT_TRUE(codepoints[0] == 0xE9);

    StringToCodepoints("\xE2\x82\xAC", codepoints); /* € U+20AC, 3 bytes */
    ASSERT_TRUE(codepoints.size() == 1);
    ASSERT_TRUE(codepoints[0] == 0x20AC);

    StringToCodepoints("\xF0\x9F\x98\x80", codepoints); /* 😀 U+1F600, 4 bytes */
    ASSERT_TRUE(codepoints.size() == 1);
    ASSERT_TRUE(codepoints[0] == 0x1F600);
}

/* Encoding codepoints produces the corresponding UTF-8 bytes with exact
   byte counts, including four-byte sequences. */
TEST(String, EncodingProducesExactUtf8Bytes) {
    std::string string;
    StringFromCodepoints({ 97, 98, 99 }, string); /* plain ASCII */
    ASSERT_TRUE(string == "abc");
    ASSERT_TRUE(string.size() == 3);

    StringFromCodepoints({ 0xE9 }, string); /* é, 2 bytes */
    ASSERT_TRUE(string == "\xC3\xA9");
    ASSERT_TRUE(string.size() == 2);

    StringFromCodepoints({ 0x20AC }, string); /* €, 3 bytes */
    ASSERT_TRUE(string == "\xE2\x82\xAC");
    ASSERT_TRUE(string.size() == 3);

    StringFromCodepoints({ 0x1F600 }, string); /* 😀, 4 bytes */
    ASSERT_TRUE(string == "\xF0\x9F\x98\x80");
    ASSERT_TRUE(string.size() == 4);

    StringFromCodepoints({ 97, 0xE9, 0x20AC, 0x1F600 }, string); /* mixed */
    ASSERT_TRUE(string == "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
    ASSERT_TRUE(string.size() == 10);
}

/* Encoding a NUL codepoint preserves the embedded NUL byte in the string. */
TEST(String, EncodingPreservesEmbeddedNul) {
    std::string string;
    StringFromCodepoints({ 97, 0, 98 }, string);
    ASSERT_TRUE(string.size() == 3);
    ASSERT_TRUE(string[0] == 'a');
    ASSERT_TRUE(string[1] == '\0');
    ASSERT_TRUE(string[2] == 'b');
}

/* Encoding an empty codepoint sequence clears the output string. */
TEST(String, EncodingEmptySequenceClearsOutput) {
    std::string string = "junk";
    StringFromCodepoints({}, string);
    ASSERT_TRUE(string.empty());
}

/* Decode/encode round-trips preserve the original string. */
TEST(String, DecodeEncodeRoundTrips) {
    std::string const original = "Hello, \xC3\xA9\xE2\x82\xAC \xF0\x9F\x98\x80 world!";
    std::vector<std::uint32_t> codepoints;
    std::string roundtrip;
    StringToCodepoints(original, codepoints);
    StringFromCodepoints(codepoints, roundtrip);
    ASSERT_TRUE(roundtrip == original);
}

/* A stray continuation byte decodes to the replacement character and
   consumes exactly one byte. */
TEST(String, StrayContinuationByteDecodesToReplacement) {
    std::vector<std::uint32_t> codepoints;
    StringToCodepoints("\x80" "abc", codepoints);
    ASSERT_TRUE(codepoints.size() == 4);
    ASSERT_TRUE(codepoints[0] == 0xFFFD);
    ASSERT_TRUE(codepoints[1] == 'a');
    ASSERT_TRUE(codepoints[2] == 'b');
    ASSERT_TRUE(codepoints[3] == 'c');
}

/* A truncated trailing sequence decodes to the replacement character
   without reading past the buffer. */
TEST(String, TruncatedTrailingSequenceDecodesToReplacement) {
    std::vector<std::uint32_t> codepoints;
    StringToCodepoints("\xE2\x82", codepoints); /* € missing its last byte */
    ASSERT_TRUE(codepoints.size() == 1);
    ASSERT_TRUE(codepoints[0] == 0xFFFD);

    StringToCodepoints("a\xF0\x9F\x98", codepoints); /* 😀 missing its last byte */
    ASSERT_TRUE(codepoints.size() == 2);
    ASSERT_TRUE(codepoints[0] == 'a');
    ASSERT_TRUE(codepoints[1] == 0xFFFD);
}

/* Inserting into the middle of an ASCII string splices at the position. */
TEST(String, InsertSplicesAtPosition) {
    std::string string = "helo";
    StringInsert(string, 2, "l");
    ASSERT_TRUE(string == "hello");
}

/* Inserting at position zero or a negative position prepends. */
TEST(String, InsertAtZeroOrNegativePrepends) {
    std::string string = "world";
    StringInsert(string, 0, "hello ");
    ASSERT_TRUE(string == "hello world");

    string = "world";
    StringInsert(string, -5, "hello ");
    ASSERT_TRUE(string == "hello world");
}

/* Inserting at or past the end appends. */
TEST(String, InsertAtOrPastEndAppends) {
    std::string string = "hello";
    StringInsert(string, 5, " world");
    ASSERT_TRUE(string == "hello world");

    string = "hello";
    StringInsert(string, 100, " world");
    ASSERT_TRUE(string == "hello world");
}

/* Inserting into an empty string sets it to the substring. */
TEST(String, InsertIntoEmptySetsSubstring) {
    std::string string;
    StringInsert(string, 0, "abc");
    ASSERT_TRUE(string == "abc");
}

/* Insertion positions are counted in codepoints, not bytes. */
TEST(String, InsertPositionsCountedInCodepoints) {
    std::string string = "h\xC3\xA9llo"; /* héllo: 5 codepoints, 6 bytes */
    StringInsert(string, 2, "xx");
    ASSERT_TRUE(string == "h\xC3\xA9xxllo");
}

/* Replace substitutes only the first occurrence. */
TEST(String, ReplaceSubstitutesFirstOccurrence) {
    ASSERT_TRUE(StringReplace("one two one", "one", "1") == "1 two one");
    ASSERT_TRUE(StringReplace("hello", "l", "L") == "heLlo");
}

/* Replace leaves the string unchanged when there is no match. */
TEST(String, ReplaceLeavesUnchangedWithoutMatch) {
    ASSERT_TRUE(StringReplace("hello", "xyz", "abc") == "hello");
    ASSERT_TRUE(StringReplace("", "xyz", "abc") == "");
}

/* Replacing with an empty string deletes the first occurrence. */
TEST(String, ReplaceWithEmptyDeletesFirstOccurrence) {
    ASSERT_TRUE(StringReplace("hello world", " world", "") == "hello");
}

/* ASCII case folding converts letters and leaves other bytes alone. */
TEST(String, AsciiCaseFolding) {
    ASSERT_TRUE(StringToLower("HeLLo, World! 123") == "hello, world! 123");
    ASSERT_TRUE(StringToUpper("HeLLo, World! 123") == "HELLO, WORLD! 123");
    ASSERT_TRUE(StringToLower("") == "");
    ASSERT_TRUE(StringToUpper("") == "");
}

/* Codepoint case mapping covers ASCII, Latin-1 and Greek. */
TEST(String, CodepointCaseMapping) {
    ASSERT_TRUE(CodepointToUpper('a') == 'A');
    ASSERT_TRUE(CodepointToUpper('z') == 'Z');
    ASSERT_TRUE(CodepointToUpper(0xE9) == 0xC9);    /* é -> É */
    ASSERT_TRUE(CodepointToUpper(0x3B1) == 0x391);  /* α -> Α */
    ASSERT_TRUE(CodepointToLower('A') == 'a');
    ASSERT_TRUE(CodepointToLower(0xC9) == 0xE9);    /* É -> é */
    ASSERT_TRUE(CodepointToLower(0x391) == 0x3B1);  /* Α -> α */
}

/* Codepoints with no case mapping are returned unchanged. */
TEST(String, UnmappedCodepointsUnchanged) {
    ASSERT_TRUE(CodepointToUpper('A') == 'A');
    ASSERT_TRUE(CodepointToUpper('5') == '5');
    ASSERT_TRUE(CodepointToLower('a') == 'a');
    ASSERT_TRUE(CodepointToLower('!') == '!');
}

/* Whitespace detection covers ASCII and Unicode spaces. */
TEST(String, WhitespaceDetection) {
    ASSERT_TRUE(CodepointIsWhitespace(' '));
    ASSERT_TRUE(CodepointIsWhitespace('\t'));
    ASSERT_TRUE(CodepointIsWhitespace('\n'));
    ASSERT_TRUE(CodepointIsWhitespace(0xA0));   /* no-break space */
    ASSERT_TRUE(CodepointIsWhitespace(0x2003)); /* em space */
    ASSERT_TRUE(CodepointIsWhitespace(0x3000)); /* ideographic space */
    ASSERT_TRUE(!CodepointIsWhitespace('a'));
    ASSERT_TRUE(!CodepointIsWhitespace('0'));
}

/* Digit detection accepts only ASCII decimal digits. */
TEST(String, DigitDetection) {
    ASSERT_TRUE(CodepointIsDigital('0'));
    ASSERT_TRUE(CodepointIsDigital('5'));
    ASSERT_TRUE(CodepointIsDigital('9'));
    ASSERT_TRUE(!CodepointIsDigital('a'));
    ASSERT_TRUE(!CodepointIsDigital(' '));
    ASSERT_TRUE(!CodepointIsDigital(0x660)); /* Arabic-Indic zero is not ASCII */
}

/* Emoji detection recognizes the major emoji ranges. */
TEST(String, EmojiDetection) {
    ASSERT_TRUE(CodepointIsEmoji(0x1F600)); /* grinning face */
    ASSERT_TRUE(CodepointIsEmoji(0x1F680)); /* rocket */
    ASSERT_TRUE(CodepointIsEmoji(0x2764));  /* heavy heart */
    ASSERT_TRUE(CodepointIsEmoji(0x1F1E6)); /* regional indicator A */
    ASSERT_TRUE(!CodepointIsEmoji('A'));
    ASSERT_TRUE(!CodepointIsEmoji(0x20AC)); /* euro sign */
}

/* Word character detection covers ASCII letters, digits, underscore and
   non-whitespace codepoints above ASCII. */
TEST(String, WordCharDetection) {
    ASSERT_TRUE(CodepointIsWordChar('a'));
    ASSERT_TRUE(CodepointIsWordChar('Z'));
    ASSERT_TRUE(CodepointIsWordChar('0'));
    ASSERT_TRUE(CodepointIsWordChar('_'));
    ASSERT_TRUE(!CodepointIsWordChar(' '));
    ASSERT_TRUE(!CodepointIsWordChar(','));
    ASSERT_TRUE(!CodepointIsWordChar('.'));
    ASSERT_TRUE(!CodepointIsWordChar('('));
    ASSERT_TRUE(CodepointIsWordChar(0xE9)); /* é */
}

/* A base codepoint with a combining mark is a single grapheme cluster. */
TEST(String, CombiningMarkFormsSingleCluster) {
    std::vector<std::uint32_t> codepoints = { 'e', 0x301 }; /* e + combining acute */
    ASSERT_TRUE(GraphemeNext(codepoints, 0) == 2);
    ASSERT_TRUE(GraphemePrev(codepoints, 2) == 0);
}

/* A ZWJ family emoji sequence is a single grapheme cluster. */
TEST(String, ZwjFamilyEmojiFormsSingleCluster) {
    std::vector<std::uint32_t> codepoints = { 0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467 }; /* 👨‍👩‍👧 */
    ASSERT_TRUE(GraphemeNext(codepoints, 0) == 5);
    ASSERT_TRUE(GraphemePrev(codepoints, 5) == 0);
}

/* A regional indicator pair is a single flag cluster. */
TEST(String, RegionalIndicatorPairFormsSingleCluster) {
    std::vector<std::uint32_t> codepoints = { 0x1F1FA, 0x1F1F8 }; /* 🇺🇸 */
    ASSERT_TRUE(GraphemeNext(codepoints, 0) == 2);
    ASSERT_TRUE(GraphemePrev(codepoints, 2) == 0);
}

/* Plain ASCII steps one codepoint per cluster and clamps at the ends. */
TEST(String, AsciiStepsOneCodepointAndClamps) {
    std::vector<std::uint32_t> codepoints = { 'a', 'b', 'c' };
    ASSERT_TRUE(GraphemeNext(codepoints, 0) == 1);
    ASSERT_TRUE(GraphemeNext(codepoints, 1) == 2);
    ASSERT_TRUE(GraphemeNext(codepoints, 2) == 3);
    ASSERT_TRUE(GraphemeNext(codepoints, 3) == 3);
    ASSERT_TRUE(GraphemePrev(codepoints, 3) == 2);
    ASSERT_TRUE(GraphemePrev(codepoints, 1) == 0);
    ASSERT_TRUE(GraphemePrev(codepoints, 0) == 0);
}
