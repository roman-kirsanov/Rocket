/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <any>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>

namespace Rocket {

/**
 * A JSON value of any kind. The active type is one of the aliases below and is
 * inspected with the IsJSON* / AsJSON* helpers (or std::any_cast directly):
 *
 *   JSON type   C++ type
 *   ---------   ------------------------------------
 *   null        JSONNull
 *   boolean     JSONBoolean  (bool)
 *   number      JSONNumber   (double)
 *   string      JSONString   (std::string)
 *   array       JSONArray    (std::vector<std::any>)
 *   object      JSONObject   (std::map<std::string, std::any>)
 *
 * ParseJSON only ever produces these six types. StringifyJSON additionally
 * accepts a few convenience types on input, see its documentation.
 */
using JSONValue = std::any;

/** The JSON null value. All instances compare equal. */
struct JSONNull {
    bool operator==(JSONNull const&) const { return true; }
    bool operator!=(JSONNull const&) const { return false; }
};

/** A JSON boolean. */
using JSONBoolean = bool;

/** A JSON number. All numbers, integral or not, are stored as doubles. */
using JSONNumber = double;

/** A JSON string, UTF-8 encoded. */
using JSONString = std::string;

/** A JSON array. Elements hold any of the JSON types. */
using JSONArray = std::vector<std::any>;

/** A JSON object. Keys are sorted; values hold any of the JSON types. */
using JSONObject = std::map<std::string, std::any>;

/**
 * Thrown by ParseJSON on malformed input and by StringifyJSON on values that
 * cannot be represented as JSON.
 */
class JSONError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * Parses a JSON document (RFC 8259) into a JSONValue.
 *
 * The whole text must be exactly one JSON value surrounded by optional
 * whitespace; a leading UTF-8 byte order mark is skipped. Strings are
 * validated as UTF-8 and \u escapes, including surrogate pairs, are decoded.
 * Numbers too large for a double are an error; numbers too small quietly
 * become zero. Comments, trailing commas, NaN, Infinity, single quotes and
 * other extensions are rejected.
 *
 * @param text The JSON text.
 * @return The parsed value, one of the JSON* types listed on JSONValue.
 * @throws JSONError describing the problem and its line and column.
 */
JSONValue ParseJSON(std::string const& text);

/**
 * Serializes a JSONValue to JSON text.
 *
 * Besides the six canonical JSON* types, the following are accepted for
 * convenience: an empty std::any and std::nullptr_t (as null), every built-in
 * integral and floating-point type (as number) and char const* (as string).
 * Numbers with an integral value below 1e21 are written as plain integers
 * (42, not 42.0); other numbers use the shortest representation that
 * round-trips. Non-finite numbers are written as null. Object keys are
 * emitted in sorted order, so output is deterministic.
 *
 * @param value  The value to serialize.
 * @param indent Spaces per nesting level. 0 (the default) yields compact
 *               single-line output; a positive value pretty-prints with
 *               newlines and that indentation.
 * @return The JSON text.
 * @throws JSONError if the value, or any nested value, holds a type not
 *         listed above, or a string that is not valid UTF-8.
 */
std::string StringifyJSON(JSONValue const& value, int indent = 0);

/**
 * Returns true if the value is JSON null: a JSONNull, a std::nullptr_t, or an
 * empty std::any (the same three that StringifyJSON writes as null).
 */
bool IsJSONNull(JSONValue const& value);

/** Returns true if the value holds a JSONBoolean (bool). */
bool IsJSONBoolean(JSONValue const& value);

/**
 * Returns true if the value holds a JSONNumber (double). Other arithmetic
 * types that StringifyJSON accepts for convenience do not count; they cannot
 * be read back with std::any_cast<JSONNumber>.
 */
bool IsJSONNumber(JSONValue const& value);

/** Returns true if the value holds a JSONString (std::string). */
bool IsJSONString(JSONValue const& value);

/** Returns true if the value holds a JSONArray. */
bool IsJSONArray(JSONValue const& value);

/** Returns true if the value holds a JSONObject. */
bool IsJSONObject(JSONValue const& value);

/**
 * Returns the boolean held by the value.
 *
 * @throws JSONError if the value is not a JSONBoolean.
 */
JSONBoolean AsJSONBoolean(JSONValue const& value);

/**
 * Returns the number held by the value.
 *
 * @throws JSONError if the value is not a JSONNumber.
 */
JSONNumber AsJSONNumber(JSONValue const& value);

/**
 * Returns a reference to the string held by the value.
 *
 * @throws JSONError if the value is not a JSONString.
 */
JSONString const& AsJSONString(JSONValue const& value);
JSONString& AsJSONString(JSONValue& value);

/**
 * Returns a reference to the array held by the value.
 *
 * @throws JSONError if the value is not a JSONArray.
 */
JSONArray const& AsJSONArray(JSONValue const& value);
JSONArray& AsJSONArray(JSONValue& value);

/**
 * Returns a reference to the object held by the value.
 *
 * @throws JSONError if the value is not a JSONObject.
 */
JSONObject const& AsJSONObject(JSONValue const& value);
JSONObject& AsJSONObject(JSONValue& value);

} /* namespace Rocket */
