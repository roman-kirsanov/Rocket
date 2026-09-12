/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Rocket {

/**
 * Decodes a UTF-8 string into a sequence of Unicode codepoints.
 *
 * @param string     The input UTF-8 string.
 * @param codepoints Output vector filled with the decoded codepoints.
 */
void StringToCodepoints(std::string const& string, std::vector<std::uint32_t>& codepoints);

/**
 * Encodes a sequence of Unicode codepoints into a UTF-8 string.
 *
 * @param codepoints The input codepoints.
 * @param string     Output string filled with the UTF-8 encoding.
 */
void StringFromCodepoints(std::vector<std::uint32_t> const& codepoints, std::string& string);

/**
 * Inserts a substring into string at the given codepoint position.
 *
 * @param string    The string to modify.
 * @param position  Codepoint index at which to insert.
 * @param substring The text to insert.
 */
void StringInsert(std::string& string, std::int64_t position, std::string const& substring);

/**
 * Returns a copy of string with the first occurrence of oldSubstring replaced by newSubstring.
 *
 * @param string       The input string.
 * @param oldSubstring The substring to search for.
 * @param newSubstring The replacement text.
 */
std::string StringReplace(std::string const& string, std::string const& oldSubstring, std::string const& newSubstring);

/**
 * Returns a lowercase copy of string using ASCII case folding only.
 *
 * @param string The input string.
 */
std::string StringToLower(std::string const& string);

/**
 * Returns an uppercase copy of string using ASCII case folding only.
 *
 * @param string The input string.
 */
std::string StringToUpper(std::string const& string);

/**
 * Returns the uppercase form of a Unicode codepoint.
 *
 * @param codepoint The codepoint to convert.
 */
std::uint32_t CodepointToUpper(std::uint32_t codepoint);

/**
 * Returns the lowercase form of a Unicode codepoint.
 *
 * @param codepoint The codepoint to convert.
 */
std::uint32_t CodepointToLower(std::uint32_t codepoint);

/**
 * Returns true if codepoint is a Unicode whitespace character.
 *
 * @param codepoint The codepoint to test.
 */
bool CodepointIsWhitespace(std::uint32_t codepoint);

/**
 * Returns true if codepoint is a word constituent for word-wise caret movement.
 *
 * @param codepoint The codepoint to test.
 */
bool CodepointIsWordChar(std::uint32_t codepoint);

/**
 * Returns true if codepoint is an ASCII decimal digit (0–9).
 *
 * @param codepoint The codepoint to test.
 */
bool CodepointIsDigital(std::uint32_t codepoint);

/**
 * Returns true if codepoint falls within a recognized emoji Unicode range.
 *
 * @param codepoint The codepoint to test.
 */
bool CodepointIsEmoji(std::uint32_t codepoint);

/**
 * Returns the index one grapheme cluster after index in codepoints (clamped to the size).
 *
 * @param codepoints The decoded codepoints.
 * @param index      The codepoint index to advance from.
 */
std::int64_t GraphemeNext(std::vector<std::uint32_t> const& codepoints, std::int64_t index);

/**
 * Returns the index one grapheme cluster before index in codepoints (clamped to 0).
 *
 * @param codepoints The decoded codepoints.
 * @param index      The codepoint index to step back from.
 */
std::int64_t GraphemePrev(std::vector<std::uint32_t> const& codepoints, std::int64_t index);

} /* namespace Rocket */
