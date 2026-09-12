/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <limits>
#include <cstring>
#include <Rocket/Base/JSON.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

namespace {

double Number(std::string const& text) {
    return std::any_cast<double>(ParseJSON(text));
}

std::string String(std::string const& text) {
    return std::any_cast<std::string>(ParseJSON(text));
}

bool Parses(std::string const& text) {
    try {
        ParseJSON(text);
        return true;
    } catch (JSONError const&) {
        return false;
    }
}

bool Stringifies(JSONValue const& value) {
    try {
        StringifyJSON(value);
        return true;
    } catch (JSONError const&) {
        return false;
    }
}

/* Builds a raw byte string; -1 marks "no byte". */
std::string Bytes(int byte1, int byte2 = -1, int byte3 = -1, int byte4 = -1) {
    std::string bytes;
    for (auto const byte : {byte1, byte2, byte3, byte4}) {
        if (byte != -1) {
            bytes += static_cast<char>(byte);
        }
    }
    return bytes;
}

} /* namespace */

/*
 * Parsing scalars
 */

/* Each scalar literal parses to its dedicated C++ type. */
TEST(JSON, ParsesScalars) {
    ASSERT_TRUE(ParseJSON("null").type() == typeid(JSONNull));
    ASSERT_TRUE(std::any_cast<bool>(ParseJSON("true")) == true);
    ASSERT_TRUE(std::any_cast<bool>(ParseJSON("false")) == false);
    ASSERT_TRUE(Number("42") == 42.0);
    ASSERT_TRUE(String("\"hi\"") == "hi");
}

/* The type aliases resolve to the types the parser actually produces. */
TEST(JSON, AliasesMatchProducedTypes) {
    ASSERT_TRUE(ParseJSON("1").type() == typeid(JSONNumber));
    ASSERT_TRUE(ParseJSON("true").type() == typeid(JSONBoolean));
    ASSERT_TRUE(ParseJSON("\"\"").type() == typeid(JSONString));
    ASSERT_TRUE(ParseJSON("[]").type() == typeid(JSONArray));
    ASSERT_TRUE(ParseJSON("{}").type() == typeid(JSONObject));
}

/* Whitespace of all four kinds is allowed around and inside values. */
TEST(JSON, SkipsWhitespace) {
    auto const value = ParseJSON(" \t\r\n{ \"a\" : [ 1 , 2 ] } \n");
    auto const& object = std::any_cast<JSONObject const&>(value);
    ASSERT_TRUE(object.size() == 1);
    ASSERT_TRUE(std::any_cast<JSONArray const&>(object.at("a")).size() == 2);
    ASSERT_TRUE(Number("\n123\n") == 123);
}

/* Numbers cover the whole RFC 8259 grammar and convert exactly. */
TEST(JSON, ParsesNumbers) {
    ASSERT_TRUE(Number("0") == 0.0);
    ASSERT_TRUE(Number("-0") == 0.0);
    ASSERT_TRUE(std::signbit(Number("-0")));
    ASSERT_TRUE(Number("-17") == -17.0);
    ASSERT_TRUE(Number("3.25") == 3.25);
    ASSERT_TRUE(Number("1e3") == 1000.0);
    ASSERT_TRUE(Number("1E+3") == 1000.0);
    ASSERT_TRUE(Number("25e-1") == 2.5);
    ASSERT_TRUE(Number("-1.5e2") == -150.0);
    ASSERT_TRUE(Number("0.1") == 0.1);
    ASSERT_TRUE(Number("9007199254740993") == 9007199254740992.0);
    ASSERT_TRUE(Number("1.7976931348623157e308") == std::numeric_limits<double>::max());
}

/* Like the reference implementation, overflow is an error but underflow
 * quietly becomes zero. */
TEST(JSON, NumberOverflowAndUnderflow) {
    ASSERT_THROW(ParseJSON("1e400"), JSONError);
    ASSERT_THROW(ParseJSON("-1e400"), JSONError);
    ASSERT_THROW(ParseJSON("[1e400]"), JSONError);
    ASSERT_TRUE(Number("1e-400") == 0.0);
    ASSERT_TRUE(Number("-1e-400") == 0.0);
    ASSERT_TRUE(std::signbit(Number("-1e-400")));
}

/* Malformed numbers are rejected. */
TEST(JSON, RejectsMalformedNumbers) {
    for (auto const* text : {"01", "-", "+1", "+0", "1.", ".5", "1e", "1e+", "0x10", "NaN", "Infinity", "-Infinity", "1_000", "1e5.5", "0b1", "-a"}) {
        ASSERT_THROW(ParseJSON(text), JSONError) << text;
    }
}

/* Partial or misspelled literals are rejected. */
TEST(JSON, RejectsBadLiterals) {
    for (auto const* text : {"nul", "tru", "fals", "True", "FALSE", "nulls", "nullx", "truee", "NULL", "None", "undefined"}) {
        ASSERT_THROW(ParseJSON(text), JSONError) << text;
    }
}

/* The IsJSON helpers identify exactly one type each, with null also
 * covering the empty and nullptr forms that stringify as null. */
TEST(JSON, IsJSONHelpers) {
    auto const values = JSONArray{JSONNull{}, true, 1.5, std::string("s"), JSONArray{}, JSONObject{}};
    auto const checks = std::vector<bool (*)(JSONValue const&)>{
        IsJSONNull, IsJSONBoolean, IsJSONNumber, IsJSONString, IsJSONArray, IsJSONObject
    };
    for (auto i = 0uz; i < values.size(); ++i) {
        for (auto j = 0uz; j < checks.size(); ++j) {
            ASSERT_TRUE(checks[j](values[i]) == (i == j)) << i << " " << j;
        }
    }
    ASSERT_TRUE(IsJSONNull(std::any{}));
    ASSERT_TRUE(IsJSONNull(nullptr));
    ASSERT_FALSE(IsJSONNumber(1));
    ASSERT_FALSE(IsJSONString("literal"));
    ASSERT_TRUE(IsJSONObject(ParseJSON("{}")));
    ASSERT_TRUE(IsJSONNumber(ParseJSON("1e3")));
    ASSERT_TRUE(IsJSONNull(ParseJSON("null")));
}

/* The AsJSON helpers return the held value and throw JSONError with a
 * descriptive message for any other type. */
TEST(JSON, AsJSONHelpers) {
    auto document = ParseJSON(R"({"flag": true, "count": 3, "name": "n", "list": [1, 2], "child": {}})");
    auto const& object = AsJSONObject(document);
    ASSERT_TRUE(AsJSONBoolean(object.at("flag")) == true);
    ASSERT_TRUE(AsJSONNumber(object.at("count")) == 3.0);
    ASSERT_TRUE(AsJSONString(object.at("name")) == "n");
    ASSERT_TRUE(AsJSONArray(object.at("list")).size() == 2);
    ASSERT_TRUE(AsJSONObject(object.at("child")).empty());

    /* The mutable overloads edit the document in place. */
    AsJSONObject(document)["added"] = 4.0;
    AsJSONArray(AsJSONObject(document).at("list")).push_back(3.0);
    AsJSONString(AsJSONObject(document).at("name")) += "ame";
    ASSERT_TRUE(StringifyJSON(document) == R"({"added":4,"child":{},"count":3,"flag":true,"list":[1,2,3],"name":"name"})");

    ASSERT_THROW(AsJSONObject(object.at("list")), JSONError);
    ASSERT_THROW(AsJSONArray(object.at("child")), JSONError);
    ASSERT_THROW(AsJSONString(object.at("count")), JSONError);
    ASSERT_THROW(AsJSONNumber(object.at("name")), JSONError);
    ASSERT_THROW(AsJSONBoolean(object.at("count")), JSONError);
    ASSERT_THROW(AsJSONNumber(1), JSONError);
    ASSERT_THROW(AsJSONObject(std::any{}), JSONError);
    try {
        AsJSONObject(object.at("list"));
        FAIL();
    } catch (JSONError const& error) {
        ASSERT_TRUE(std::string(error.what()) == "expected a JSON object, got an array") << error.what();
    }
}

/*
 * Parsing strings
 */

/* Every simple escape decodes to its character. */
TEST(JSON, DecodesSimpleEscapes) {
    ASSERT_TRUE(String(R"("\" \\ \/ \b \f \n \r \t")") == "\" \\ / \b \f \n \r \t");
}

/* Unicode escapes decode to UTF-8, including surrogate pairs and NUL. */
TEST(JSON, DecodesUnicodeEscapes) {
    ASSERT_TRUE(String(R"("A")") == "A");
    ASSERT_TRUE(String("\"\\u00e9\"") == "\xC3\xA9");
    ASSERT_TRUE(String("\"\\u20ac\"") == "\xE2\x82\xAC");
    ASSERT_TRUE(String("\"\\ud83d\\ude00\"") == "\xF0\x9F\x98\x80");
    ASSERT_TRUE(String("\"\\uD83D\\uDE00\"") == "\xF0\x9F\x98\x80");
    ASSERT_TRUE(String("\"\\u0000\"") == std::string("\0", 1));
    ASSERT_TRUE(String("\"a\\u0000b\"") == std::string("a\0b", 3));
}

/* Raw UTF-8 passes through untouched. */
TEST(JSON, PassesThroughUtf8) {
    auto const text = std::string("\"caf\xC3\xA9 \xE2\x82\xAC \xF0\x9F\x98\x80\"");
    ASSERT_TRUE(String(text) == "caf\xC3\xA9 \xE2\x82\xAC \xF0\x9F\x98\x80");
}

/* Malformed strings are rejected. */
TEST(JSON, RejectsMalformedStrings) {
    ASSERT_THROW(ParseJSON("\"abc"), JSONError);
    ASSERT_THROW(ParseJSON("\"a\\"), JSONError);
    ASSERT_THROW(ParseJSON("'single'"), JSONError);
    ASSERT_THROW(ParseJSON("\"tab\there\""), JSONError);
    ASSERT_THROW(ParseJSON("\"new\nline\""), JSONError);
    ASSERT_THROW(ParseJSON(R"("\x41")"), JSONError);
    ASSERT_THROW(ParseJSON(R"("\u12")"), JSONError);
    ASSERT_THROW(ParseJSON(R"("\u12G4")"), JSONError);
    ASSERT_THROW(ParseJSON(R"("\ud83d")"), JSONError);
    ASSERT_THROW(ParseJSON(R"("\ud83dx")"), JSONError);
    ASSERT_THROW(ParseJSON(R"("\ud83dA")"), JSONError);
    ASSERT_THROW(ParseJSON(R"("\ude00")"), JSONError);
    ASSERT_THROW(ParseJSON(R"("\U0041")"), JSONError);
}

/* Invalid UTF-8 byte sequences are rejected. */
TEST(JSON, RejectsInvalidUtf8) {
    ASSERT_THROW(ParseJSON("\"\xFF\""), JSONError);
    ASSERT_THROW(ParseJSON("\"\xC3\""), JSONError);
    ASSERT_THROW(ParseJSON("\"\xC0\x80\""), JSONError);
    ASSERT_THROW(ParseJSON("\"\xED\xA0\x80\""), JSONError);
    ASSERT_THROW(ParseJSON("\"\xF4\x90\x80\x80\""), JSONError);
    ASSERT_THROW(ParseJSON("\"\x80\""), JSONError);
}

/*
 * Parsing containers
 */

/* Objects map keys to values, with keys sorted and later duplicates winning. */
TEST(JSON, ParsesObjects) {
    auto const value = ParseJSON(R"({"b": 2, "a": 1, "c": {"d": null}, "a": 3})");
    auto const& object = std::any_cast<JSONObject const&>(value);
    ASSERT_TRUE(object.size() == 3);
    ASSERT_TRUE(object.begin()->first == "a");
    ASSERT_TRUE(std::any_cast<double>(object.at("a")) == 3.0);
    ASSERT_TRUE(std::any_cast<double>(object.at("b")) == 2.0);
    auto const& inner = std::any_cast<JSONObject const&>(object.at("c"));
    ASSERT_TRUE(inner.at("d").type() == typeid(JSONNull));

    auto const empty = std::any_cast<JSONObject>(ParseJSON("{\"\": true, \"one\": 1, \"two\": null}"));
    ASSERT_TRUE(empty.size() == 3);
    ASSERT_TRUE(std::any_cast<bool>(empty.at("")) == true);
}

/* Arrays keep order and mix element types. */
TEST(JSON, ParsesArrays) {
    auto const value = ParseJSON(R"([1, "two", true, null, [3], {"four": 4}])");
    auto const& array = std::any_cast<JSONArray const&>(value);
    ASSERT_TRUE(array.size() == 6);
    ASSERT_TRUE(std::any_cast<double>(array[0]) == 1.0);
    ASSERT_TRUE(std::any_cast<std::string>(array[1]) == "two");
    ASSERT_TRUE(std::any_cast<bool>(array[2]) == true);
    ASSERT_TRUE(array[3].type() == typeid(JSONNull));
    ASSERT_TRUE(std::any_cast<JSONArray const&>(array[4]).size() == 1);
    ASSERT_TRUE(std::any_cast<JSONObject const&>(array[5]).size() == 1);
}

/* Empty containers parse, with or without inner whitespace. */
TEST(JSON, ParsesEmptyContainers) {
    ASSERT_TRUE(std::any_cast<JSONObject const&>(ParseJSON("{}")).empty());
    ASSERT_TRUE(std::any_cast<JSONObject const&>(ParseJSON("{ }")).empty());
    ASSERT_TRUE(std::any_cast<JSONObject const&>(ParseJSON("{ \n }")).empty());
    ASSERT_TRUE(std::any_cast<JSONArray const&>(ParseJSON("[]")).empty());
    ASSERT_TRUE(std::any_cast<JSONArray const&>(ParseJSON("[ ]")).empty());
    ASSERT_TRUE(std::any_cast<JSONArray const&>(ParseJSON("[ \n ]")).empty());
}

/* Structural mistakes and common extensions are rejected. */
TEST(JSON, RejectsMalformedContainers) {
    for (auto const* text : {
        "{", "[", "}", "]", "[1,]", "[,1]", "[1 2]", "[,]", "{,}", "[1,,2]",
        "{\"a\":1,}", "{\"a\" 1}", "{\"a\":}", "{a: 1}", "{1: 1}", "{\"a\": 1 \"b\": 2}",
        "{\"a\":1,,\"b\":2}", "{\"a\"::1}", "[1] // comment", "/* c */ [1]", "[1", "[1,", "{\"a\":1",
        "[\"a\", ]", "[1}", "{\"a\":1]"
    }) {
        ASSERT_THROW(ParseJSON(text), JSONError) << text;
    }
}

/* The document must be exactly one value. */
TEST(JSON, RejectsEmptyOrMultipleDocuments) {
    ASSERT_THROW(ParseJSON(""), JSONError);
    ASSERT_THROW(ParseJSON("   "), JSONError);
    ASSERT_THROW(ParseJSON("1 2"), JSONError);
    ASSERT_THROW(ParseJSON("{} {}"), JSONError);
    ASSERT_THROW(ParseJSON("[1] x"), JSONError);
    ASSERT_THROW(ParseJSON("\uFF01"), JSONError);
    ASSERT_THROW(ParseJSON("[-4:1,]"), JSONError);
}

/* Errors report a line and column and describe the problem. */
TEST(JSON, ErrorsReportPosition) {
    try {
        ParseJSON("{\n  \"a\": 1,\n  \"b\": tru\n}");
        FAIL();
    } catch (JSONError const& error) {
        auto const message = std::string(error.what());
        ASSERT_TRUE(message.find("line 3") != std::string::npos) << message;
        ASSERT_TRUE(message.find("column 8") != std::string::npos) << message;
        ASSERT_TRUE(message.find("'true'") != std::string::npos) << message;
    }
    try {
        ParseJSON("[1, 2, 3");
        FAIL();
    } catch (JSONError const& error) {
        auto const message = std::string(error.what());
        ASSERT_TRUE(message.find("line 1") != std::string::npos) << message;
        ASSERT_TRUE(message.find("column 9") != std::string::npos) << message;
        ASSERT_TRUE(message.find("end of input") != std::string::npos) << message;
    }
}

/* JSONError is a std::runtime_error, so generic handlers catch it too. */
TEST(JSON, ErrorIsRuntimeError) {
    ASSERT_THROW(ParseJSON("["), std::runtime_error);
    ASSERT_THROW(StringifyJSON(std::string("\xFF")), std::runtime_error);
}

/* Runaway nesting is rejected instead of overflowing the stack. */
TEST(JSON, RejectsExcessiveNesting) {
    ASSERT_NO_THROW(ParseJSON(std::string(1000, '[') + std::string(1000, ']')));
    ASSERT_THROW(ParseJSON(std::string(100000, '[')), JSONError);
    ASSERT_THROW(ParseJSON(std::string(100000, '[') + std::string(100000, ']')), JSONError);
    ASSERT_THROW(ParseJSON(std::string(2000, '[')), JSONError);
}

/*
 * Stringifying
 */

/* Scalars serialize to their literals. */
TEST(JSON, StringifiesScalars) {
    ASSERT_TRUE(StringifyJSON(JSONNull{}) == "null");
    ASSERT_TRUE(StringifyJSON(true) == "true");
    ASSERT_TRUE(StringifyJSON(false) == "false");
    ASSERT_TRUE(StringifyJSON(1.5) == "1.5");
    ASSERT_TRUE(StringifyJSON(std::string("x")) == "\"x\"");
}

/* Integral doubles print without a fraction; others use the shortest
 * round-tripping form; non-finite values become null. */
TEST(JSON, StringifiesNumbers) {
    ASSERT_TRUE(StringifyJSON(0.0) == "0");
    ASSERT_TRUE(StringifyJSON(-0.0) == "0");
    ASSERT_TRUE(StringifyJSON(42.0) == "42");
    ASSERT_TRUE(StringifyJSON(-42.0) == "-42");
    ASSERT_TRUE(StringifyJSON(0.1) == "0.1");
    ASSERT_TRUE(StringifyJSON(-2.5) == "-2.5");
    ASSERT_TRUE(StringifyJSON(1e20) == "100000000000000000000");
    ASSERT_TRUE(StringifyJSON(1e21) == "1e+21");
    ASSERT_TRUE(StringifyJSON(9007199254740992.0) == "9007199254740992");
    ASSERT_TRUE(StringifyJSON(1e-7) == "1e-07");
    ASSERT_TRUE(StringifyJSON(std::numeric_limits<double>::infinity()) == "null");
    ASSERT_TRUE(StringifyJSON(-std::numeric_limits<double>::infinity()) == "null");
    ASSERT_TRUE(StringifyJSON(std::numeric_limits<double>::quiet_NaN()) == "null");
}

/* Strings escape quotes, backslashes and control characters, and pass
 * through non-ASCII UTF-8 and the solidus untouched. */
TEST(JSON, StringifiesStrings) {
    ASSERT_TRUE(StringifyJSON(std::string("a\"b\\c/d")) == R"("a\"b\\c/d")");
    ASSERT_TRUE(StringifyJSON(std::string("\b\f\n\r\t")) == R"("\b\f\n\r\t")");
    ASSERT_TRUE(StringifyJSON(std::string("\x01\x1F")) == "\"\\u0001\\u001f\"");
    ASSERT_TRUE(StringifyJSON(std::string("\0", 1)) == "\"\\u0000\"");
    ASSERT_TRUE(StringifyJSON(std::string("caf\xC3\xA9 \xF0\x9F\x98\x80")) == "\"caf\xC3\xA9 \xF0\x9F\x98\x80\"");
    ASSERT_TRUE(StringifyJSON(std::string("\x7F")) == "\"\x7F\"");
}

/* Strings that are not valid UTF-8 cannot be stringified. */
TEST(JSON, StringifyRejectsInvalidUtf8) {
    ASSERT_THROW(StringifyJSON(std::string("\xFF")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("ab\xC3")), JSONError);
    ASSERT_THROW(StringifyJSON(JSONObject{{"\xFF", 1.0}}), JSONError);
    ASSERT_THROW(StringifyJSON(JSONArray{std::string("\xC0\x80")}), JSONError);
}

/* Compact output has no whitespace and sorted keys. */
TEST(JSON, StringifiesContainersCompactly) {
    auto const value = JSONObject{
        {"z", JSONArray{1.0, std::string("two"), true, JSONNull{}}},
        {"a", JSONObject{{"nested", JSONArray{}}}},
        {"m", JSONObject{}},
    };
    ASSERT_TRUE(StringifyJSON(value) == R"({"a":{"nested":[]},"m":{},"z":[1,"two",true,null]})");
    ASSERT_TRUE(StringifyJSON(JSONArray{std::string("foo"), 1.0, 2.0, 3.0, false, JSONObject{{"one", 1.0}}}) == R"(["foo",1,2,3,false,{"one":1}])");
}

/* Pretty output uses newlines and the requested indentation. */
TEST(JSON, StringifiesContainersPrettily) {
    auto const value = JSONObject{
        {"a", JSONArray{1.0, 2.0}},
        {"b", JSONObject{}},
        {"c", JSONArray{}},
        {"d", JSONObject{{"e", std::string("f")}}},
    };
    auto const expected =
        "{\n"
        "  \"a\": [\n"
        "    1,\n"
        "    2\n"
        "  ],\n"
        "  \"b\": {},\n"
        "  \"c\": [],\n"
        "  \"d\": {\n"
        "    \"e\": \"f\"\n"
        "  }\n"
        "}";
    ASSERT_TRUE(StringifyJSON(value, 2) == expected);
    ASSERT_TRUE(StringifyJSON(JSONArray{}, 4) == "[]");
    ASSERT_TRUE(StringifyJSON(JSONArray{1.0}, 4) == "[\n    1\n]");
    ASSERT_TRUE(StringifyJSON(value, 0) == StringifyJSON(value));
    ASSERT_TRUE(StringifyJSON(value, -1) == StringifyJSON(value));
}

/* Convenience input types are accepted when building values by hand. */
TEST(JSON, StringifiesConvenienceTypes) {
    ASSERT_TRUE(StringifyJSON(std::any{}) == "null");
    ASSERT_TRUE(StringifyJSON(nullptr) == "null");
    ASSERT_TRUE(StringifyJSON(7) == "7");
    ASSERT_TRUE(StringifyJSON(-7) == "-7");
    ASSERT_TRUE(StringifyJSON(7u) == "7");
    ASSERT_TRUE(StringifyJSON(7L) == "7");
    ASSERT_TRUE(StringifyJSON(7LL) == "7");
    ASSERT_TRUE(StringifyJSON(7ULL) == "7");
    ASSERT_TRUE(StringifyJSON(static_cast<short>(7)) == "7");
    ASSERT_TRUE(StringifyJSON(static_cast<unsigned char>(7)) == "7");
    ASSERT_TRUE(StringifyJSON(2.5f) == "2.5");
    ASSERT_TRUE(StringifyJSON(2.5L) == "2.5");
    ASSERT_TRUE(StringifyJSON("literal") == "\"literal\"");
    ASSERT_TRUE(StringifyJSON(JSONArray{1, "x", nullptr}) == R"([1,"x",null])");
}

/* Types that have no JSON representation are rejected. */
TEST(JSON, StringifyRejectsUnknownTypes) {
    struct Opaque {};
    ASSERT_THROW(StringifyJSON(Opaque{}), JSONError);
    ASSERT_THROW(StringifyJSON(JSONArray{1.0, Opaque{}}), JSONError);
    ASSERT_THROW(StringifyJSON(JSONObject{{"k", Opaque{}}}), JSONError);
    ASSERT_THROW(StringifyJSON(std::vector<int>{1}), JSONError);
}

/* Parsing the stringified form gives back the same document. */
TEST(JSON, RoundTrips) {
    auto const text = R"({"array":[1,2.5,-3,1e+21,"s",true,false,null],"nested":{"deep":{"deeper":[[],{}]}},"text":"quote \" backslash \\ tab \t emoji 😀 é"})";
    auto const once = StringifyJSON(ParseJSON(text));
    auto const twice = StringifyJSON(ParseJSON(once));
    ASSERT_TRUE(once == twice);
    ASSERT_TRUE(once == "{\"array\":[1,2.5,-3,1e+21,\"s\",true,false,null],\"nested\":{\"deep\":{\"deeper\":[[],{}]}},\"text\":\"quote \\\" backslash \\\\ tab \\t emoji \xF0\x9F\x98\x80 \xC3\xA9\"}");
    ASSERT_TRUE(StringifyJSON(ParseJSON(StringifyJSON(ParseJSON(text), 3))) == once);
}

/* Doubles survive a round trip bit for bit. */
TEST(JSON, NumbersRoundTripExactly) {
    for (auto const value : {0.1, 1.0 / 3.0, 123456789.123456789, 5e-324, 1.7976931348623157e308, -0.000001, 2.2250738585072014e-308, 1e21, 123456789012345680000.0}) {
        ASSERT_TRUE(Number(StringifyJSON(value)) == value) << value;
    }
}

/*
 * Ported from the reference implementation's tests (nlohmann/json,
 * tests/src/unit-class_parser.cpp).
 */

/* Every control character inside a string must be escaped. */
TEST(JSONReference, ParserRejectsRawControlCharacters) {
    for (auto c = 0; c < 0x20; ++c) {
        auto const text = "\"" + std::string(1, static_cast<char>(c)) + "\"";
        ASSERT_THROW(ParseJSON(text), JSONError) << c;
    }
    /* A NUL byte in the middle of an otherwise fine string. */
    std::string s = "\"1\"";
    s[1] = '\0';
    ASSERT_THROW(ParseJSON(s), JSONError);
}

/* The escaped strings table from the reference parser tests. */
TEST(JSONReference, ParserDecodesEscapedStrings) {
    ASSERT_TRUE(String("\"\\\"\"") == "\"");
    ASSERT_TRUE(String("\"\\\\\"") == "\\");
    ASSERT_TRUE(String("\"\\/\"") == "/");
    ASSERT_TRUE(String("\"\\b\"") == "\b");
    ASSERT_TRUE(String("\"\\f\"") == "\f");
    ASSERT_TRUE(String("\"\\n\"") == "\n");
    ASSERT_TRUE(String("\"\\r\"") == "\r");
    ASSERT_TRUE(String("\"\\t\"") == "\t");
    ASSERT_TRUE(String("\"\\u0001\"") == "\x01");
    ASSERT_TRUE(String("\"\\u000a\"") == "\n");
    ASSERT_TRUE(String("\"\\u00b0\"") == "°");
    ASSERT_TRUE(String("\"\\u0c00\"") == "ఀ");
    ASSERT_TRUE(String("\"\\ud000\"") == "퀀");
    ASSERT_TRUE(String("\"\\u000E\"") == "\x0E");
    ASSERT_TRUE(String("\"\\u00F0\"") == "ð");
    ASSERT_TRUE(String("\"\\u0100\"") == "Ā");
    ASSERT_TRUE(String("\"\\u2000\"") == "\xE2\x80\x80");
    ASSERT_TRUE(String("\"\\uFFFF\"") == "\xEF\xBF\xBF");
    ASSERT_TRUE(String("\"\\u20AC\"") == "€");
    ASSERT_TRUE(String("\"€\"") == "€");
    ASSERT_TRUE(String("\"🎈\"") == "🎈");
    ASSERT_TRUE(String("\"\\ud80c\\udc60\"") == "\xf0\x93\x81\xa0");
    ASSERT_TRUE(String("\"\\ud83c\\udf1e\"") == "🌞");
}

/* The number tables from the reference parser tests. */
TEST(JSONReference, ParserReadsNumbers) {
    ASSERT_TRUE(Number("-128") == -128);
    ASSERT_TRUE(Number("-0") == 0);
    ASSERT_TRUE(Number("0") == 0);
    ASSERT_TRUE(Number("128") == 128);
    ASSERT_TRUE(Number("0e1") == 0e1);
    ASSERT_TRUE(Number("0E1") == 0e1);
    for (auto exponent = -4; exponent <= 4; ++exponent) {
        auto const expected = 10000 * std::pow(10.0, exponent);
        ASSERT_TRUE(Number("10000E" + std::to_string(exponent)) == expected) << exponent;
        ASSERT_TRUE(Number("10000e" + std::to_string(exponent)) == expected) << exponent;
    }
    ASSERT_TRUE(Number("-0e1") == -0e1);
    ASSERT_TRUE(Number("-0E1") == -0e1);
    ASSERT_TRUE(Number("-0E123") == -0e123);
    for (auto exponent = 0; exponent <= 9; ++exponent) {
        auto const expected = 10 * std::pow(10.0, exponent);
        ASSERT_TRUE(Number("10E" + std::to_string(exponent)) == expected) << exponent;
        ASSERT_TRUE(Number("10E+" + std::to_string(exponent)) == expected) << exponent;
    }
    for (auto exponent = 1; exponent <= 9; ++exponent) {
        auto const expected = std::stod("10E-" + std::to_string(exponent));
        ASSERT_TRUE(Number("10E-" + std::to_string(exponent)) == expected) << exponent;
    }
    ASSERT_TRUE(Number("-9007199254740991") == -9007199254740991.0);
    ASSERT_TRUE(Number("9007199254740991") == 9007199254740991.0);
    ASSERT_TRUE(Number("-9223372036854775808") == -9223372036854775808.0);
    ASSERT_TRUE(Number("9223372036854775807") == 9223372036854775807.0);
    ASSERT_TRUE(Number("18446744073709551615") == 18446744073709551615.0);
    ASSERT_TRUE(Number("-128.5") == -128.5);
    ASSERT_TRUE(Number("0.999") == 0.999);
    ASSERT_TRUE(Number("128.5") == 128.5);
    ASSERT_TRUE(Number("-0.0") == -0.0);
    ASSERT_TRUE(Number("-128.5E3") == -128.5E3);
    ASSERT_TRUE(Number("-128.5E-3") == -128.5E-3);
    ASSERT_TRUE(Number("-0.0e1") == -0.0e1);
    ASSERT_TRUE(Number("-0.0E1") == -0.0e1);
    ASSERT_THROW(ParseJSON("1.18973e+4932"), JSONError);
}

/* The invalid number table from the reference parser tests. */
TEST(JSONReference, ParserRejectsInvalidNumbers) {
    for (auto const* text : {
        "01", "-01", "--1", "1.", "1E", "1E-", "1.E1", "-1E", "-0E#", "-0E-#", "-0#", "-0.0:", "-0.0Z",
        "-0E123:", "-0e0-:", "-0e-:", "-0f", "0.", "-", "--", "-0.", "-.", "-:", "0.:", "e.", "1e.",
        "1e/", "1e:", "1E.", "1E/", "1E:", "+1", "+0"
    }) {
        ASSERT_THROW(ParseJSON(text), JSONError) << text;
    }
}

/* Every backslash-character pair other than the eight simple escapes is an
 * error; \u is covered separately. */
TEST(JSONReference, ParserRejectsInvalidEscapes) {
    for (auto c = 1; c < 128; ++c) {
        auto const text = std::string("\"\\") + static_cast<char>(c) + "\"";
        switch (c) {
            case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't':
                ASSERT_NO_THROW(ParseJSON(text)) << c;
                break;
            case 'u':
                break;
            default:
                ASSERT_THROW(ParseJSON(text), JSONError) << c;
                break;
        }
    }
}

/* Each of the four \u digits must be a hex digit. */
TEST(JSONReference, ParserRejectsInvalidUnicodeEscapes) {
    auto const valid = [](int c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    };
    for (auto c = 1; c < 128; ++c) {
        auto const ch = std::string(1, static_cast<char>(c));
        auto const s1 = "\"\\u000" + ch + "\"";
        auto const s2 = "\"\\u00" + ch + "0\"";
        auto const s3 = "\"\\u0" + ch + "00\"";
        auto const s4 = "\"\\u" + ch + "000\"";
        for (auto const& text : {s1, s2, s3, s4}) {
            if (valid(c)) {
                ASSERT_NO_THROW(ParseJSON(text)) << text;
            } else {
                ASSERT_THROW(ParseJSON(text), JSONError) << text;
            }
        }
    }
}

/* The parse error table from the reference parser tests. */
TEST(JSONReference, ParserReportsParseErrors) {
    for (auto const* text : {
        "n", "nu", "nul", "nulk", "nulm",
        "t", "tr", "tru", "trud", "truf",
        "f", "fa", "fal", "fals", "falsd", "falsf",
        "[", "[1", "[1,", "[1,]", "]",
        "{", "{\"foo\"", "{\"foo\":", "{\"foo\":}", "{\"foo\":1,}", "}",
        "\"", "\"\\\"", "\"\\u\"", "\"\\u0\"", "\"\\u01\"", "\"\\u012\"",
        "\"\\u", "\"\\u0", "\"\\u01", "\"\\u012",
        "\"\\uD80C\"", "\"\\uD80C\\uD80C\"", "\"\\uD80C\\u0000\"", "\"\\uD80C\\uFFFF\"",
        "{,\"key\": false}", "[{\"key\": false true]", "{\"foo\": true:",
        "[1,2,x]", "  \n  @", "{\"a\": }", "[1 2]", "\xEF\xBB\xBF   nul"
    }) {
        ASSERT_THROW(ParseJSON(text), JSONError) << text;
    }
}

/* Comments and trailing commas are not part of JSON. The reference tests
 * apply these decorations to every accepted document. */
TEST(JSONReference, ParserRejectsCommentsAndTrailingCommas) {
    for (auto const* text : {
        "null", "true", "false", "[]", "[ ]", "[true, false, null]", "{}", "{ }",
        "{\"\": true, \"one\": 1, \"two\": null}", "\"\"", "128", "-128.5E-3"
    }) {
        auto const s = std::string(text);
        ASSERT_NO_THROW(ParseJSON(s)) << text;
        ASSERT_THROW(ParseJSON("// this is a comment\n" + s), JSONError) << text;
        ASSERT_THROW(ParseJSON("/* this is a comment */" + s), JSONError) << text;
        ASSERT_THROW(ParseJSON(s + "// this is a comment"), JSONError) << text;
        ASSERT_THROW(ParseJSON(s + "/* this is a comment */"), JSONError) << text;
        if (s.size() > 2 && (s.back() == ']' || s.back() == '}')) {
            auto const body = s.substr(0, s.size() - 1);
            ASSERT_THROW(ParseJSON(body + " ," + s.back()), JSONError) << text;
            ASSERT_THROW(ParseJSON(body + "," + s.back()), JSONError) << text;
            ASSERT_THROW(ParseJSON(body + ", " + s.back()), JSONError) << text;
        }
    }
    ASSERT_THROW(ParseJSON("/a"), JSONError);
    ASSERT_THROW(ParseJSON("/*"), JSONError);
}

/*
 * Ported from tests/src/unit-deserialization.cpp.
 */

/* Truncated and ill-formed byte sequences from the reference error cases. */
TEST(JSONReference, DeserializationErrorCases) {
    ASSERT_THROW(ParseJSON("\"aaaaaa\\u"), JSONError);
    ASSERT_THROW(ParseJSON("\"aaaaaa\\u1"), JSONError);
    ASSERT_THROW(ParseJSON("\"aaaaaa\\u11111111"), JSONError);
    ASSERT_THROW(ParseJSON("\"aaaaaau11111111\\"), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xC1)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xDF, 0x7F)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xDF, 0xC0)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xE0, 0x9F)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xEF, 0xC0)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xED, 0x7F)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xF0, 0x8F)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xF0, 0xC0)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xF3, 0x7F)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xF3, 0xC0)), JSONError);
    ASSERT_THROW(ParseJSON(Bytes('"', 0x7F, 0xF4, 0x7F)), JSONError);
    ASSERT_THROW(ParseJSON("{\"\":11"), JSONError);
    ASSERT_THROW(ParseJSON("{\"foo\": true, \"bar\": 1}}"), JSONError);
    ASSERT_THROW(ParseJSON(""), JSONError);
}

/* A UTF-8 byte order mark is skipped, but only the complete, exact one. */
TEST(JSONReference, ByteOrderMark) {
    auto const bom = std::string("\xEF\xBB\xBF");
    ASSERT_THROW(ParseJSON(bom), JSONError);
    ASSERT_TRUE(Number(bom + "1") == 1);
    ASSERT_TRUE(ParseJSON(bom + "null").type() == typeid(JSONNull));
    ASSERT_THROW(ParseJSON(bom.substr(0, 2)), JSONError);
    ASSERT_THROW(ParseJSON(bom.substr(0, 1)), JSONError);
    ASSERT_THROW(ParseJSON(bom.substr(0, 2) + "1"), JSONError);
    ASSERT_THROW(ParseJSON(bom.substr(0, 1) + "1"), JSONError);
    ASSERT_THROW(ParseJSON(bom + bom + "1"), JSONError);
    ASSERT_THROW(ParseJSON(" " + bom + "1"), JSONError);
    for (auto i0 = 0; i0 < 3; ++i0) {
        for (auto i1 = 0; i1 < 3; ++i1) {
            for (auto i2 = 0; i2 < 3; ++i2) {
                std::string s;
                s.push_back(static_cast<char>(bom[0] + i0));
                s.push_back(static_cast<char>(bom[1] + i1));
                s.push_back(static_cast<char>(bom[2] + i2));
                if (i0 == 0 && i1 == 0 && i2 == 0) {
                    ASSERT_TRUE(ParseJSON(s + "null").type() == typeid(JSONNull));
                } else {
                    ASSERT_THROW(ParseJSON(s + "null"), JSONError) << i0 << i1 << i2;
                }
            }
        }
    }
}

/*
 * Ported from tests/src/unit-testsuites.cpp (the parts that do not need the
 * external test data download).
 */

/* Double parsing cases from nativejson-benchmark. */
TEST(JSONReference, NativeJsonBenchmarkDoubles) {
    auto const check = [](std::string const& text, double expected) {
        auto const array = std::any_cast<JSONArray>(ParseJSON(text));
        ASSERT_TRUE(array.size() == 1) << text;
        ASSERT_TRUE(std::any_cast<double>(array[0]) == expected) << text;
    };
    check("[0.0]", 0.0);
    check("[-0.0]", -0.0);
    check("[1.0]", 1.0);
    check("[-1.0]", -1.0);
    check("[1.5]", 1.5);
    check("[-1.5]", -1.5);
    check("[3.1416]", 3.1416);
    check("[1E10]", 1E10);
    check("[1e10]", 1e10);
    check("[1E+10]", 1E+10);
    check("[1E-10]", 1E-10);
    check("[-1E10]", -1E10);
    check("[-1e10]", -1e10);
    check("[-1E+10]", -1E+10);
    check("[-1E-10]", -1E-10);
    check("[1.234E+10]", 1.234E+10);
    check("[1.234E-10]", 1.234E-10);
    check("[1.79769e+308]", 1.79769e+308);
    check("[2.22507e-308]", 2.22507e-308);
    check("[-1.79769e+308]", -1.79769e+308);
    check("[-2.22507e-308]", -2.22507e-308);
    check("[4.9406564584124654e-324]", 4.9406564584124654e-324);
    check("[2.2250738585072009e-308]", 2.2250738585072009e-308);
    check("[2.2250738585072014e-308]", 2.2250738585072014e-308);
    check("[1.7976931348623157e+308]", 1.7976931348623157e+308);
    check("[1e-10000]", 0.0);
    check("[18446744073709551616]", 18446744073709551616.0);
    check("[-9223372036854775809]", -9223372036854775809.0);
    check("[0.9868011474609375]", 0.9868011474609375);
    check("[123e34]", 123e34);
    check("[45913141877270640000.0]", 45913141877270640000.0);
    check("[2.2250738585072011e-308]", 2.2250738585072011e-308);
    check("[1e-214748363]", 0.0);
    check("[1e-214748364]", 0.0);
    check("[0.017976931348623157e+310]", 1.7976931348623157e+308);
    check("[2.2250738585072012e-308]", 2.2250738585072014e-308);
    check("[2.22507385850720113605740979670913197593481954635164564e-308]", 2.2250738585072009e-308);
    check("[2.22507385850720113605740979670913197593481954635164565e-308]", 2.2250738585072014e-308);
    check("[0.999999999999999944488848768742172978818416595458984375]", 1.0);
    check("[0.999999999999999944488848768742172978818416595458984374]", 0.99999999999999989);
    check("[0.999999999999999944488848768742172978818416595458984376]", 1.0);
    check("[1.00000000000000011102230246251565404236316680908203125]", 1.0);
    check("[1.00000000000000011102230246251565404236316680908203124]", 1.0);
    check("[1.00000000000000011102230246251565404236316680908203126]", 1.00000000000000022);
    check("[72057594037927928.0]", 72057594037927928.0);
    check("[72057594037927936.0]", 72057594037927936.0);
    check("[72057594037927932.0]", 72057594037927936.0);
    check("[7205759403792793199999e-5]", 72057594037927928.0);
    check("[7205759403792793200001e-5]", 72057594037927936.0);
    check("[9223372036854774784.0]", 9223372036854774784.0);
    check("[9223372036854775808.0]", 9223372036854775808.0);
    check("[9223372036854775296.0]", 9223372036854775808.0);
    check("[922337203685477529599999e-5]", 9223372036854774784.0);
    check("[922337203685477529600001e-5]", 9223372036854775808.0);
    check("[10141204801825834086073718800384]", 10141204801825834086073718800384.0);
    check("[10141204801825835211973625643008]", 10141204801825835211973625643008.0);
    check("[10141204801825834649023672221696]", 10141204801825835211973625643008.0);
    check("[1014120480182583464902367222169599999e-5]", 10141204801825834086073718800384.0);
    check("[1014120480182583464902367222169600001e-5]", 10141204801825835211973625643008.0);
    check("[5708990770823838890407843763683279797179383808]", 5708990770823838890407843763683279797179383808.0);
    check("[5708990770823839524233143877797980545530986496]", 5708990770823839524233143877797980545530986496.0);
    check("[5708990770823839207320493820740630171355185152]", 5708990770823839524233143877797980545530986496.0);
    check("[5708990770823839207320493820740630171355185151999e-3]", 5708990770823838890407843763683279797179383808.0);
    check("[5708990770823839207320493820740630171355185152001e-3]", 5708990770823839524233143877797980545530986496.0);
    {
        std::string n1e308(311, '0');
        n1e308[0] = '[';
        n1e308[1] = '1';
        n1e308[310] = ']';
        check(n1e308, 1E308);
    }
    check(
        "[2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508"
        "7914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012"
        "9811224514518898490572223072852551331557550159143974763979834118019993239625482890171070818506906306"
        "6665599493827577257201576306269066333264756530000924588831643303777979186961204949739037782970490505"
        "1080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621"
        "5722898802581825451803257070188608721131280795122334262883686223215037756666225039825343359745688844"
        "2390026549819838548794829220689472168983109969836584681402285424333066033985088644580400103493397042"
        "7567186443383770486037861622771738545623065874679014086723327636718751234567890123456789012345678901"
        "e-308]",
        2.2250738585072014e-308);
}

/* String parsing cases from nativejson-benchmark. */
TEST(JSONReference, NativeJsonBenchmarkStrings) {
    auto const check = [](std::string const& text, std::string const& expected) {
        auto const array = std::any_cast<JSONArray>(ParseJSON(text));
        ASSERT_TRUE(array.size() == 1) << text;
        ASSERT_TRUE(std::any_cast<std::string>(array[0]) == expected) << text;
    };
    check("[\"\"]", "");
    check("[\"Hello\"]", "Hello");
    check(R"(["Hello\nWorld"])", "Hello\nWorld");
    check("[\"Hello\\u0000World\"]", std::string("Hello\0World", 11));
    check(R"(["\"\\/\b\f\n\r\t"])", "\"\\/\b\f\n\r\t");
    check(R"(["\u0024"])", "$");
    check(R"(["\u00A2"])", "\xC2\xA2");
    check(R"(["\u20AC"])", "\xE2\x82\xAC");
    check(R"(["\uD834\uDD1E"])", "\xF0\x9D\x84\x9E");
}

/* The examples from RFC 8259 itself. */
TEST(JSONReference, Rfc8259Examples) {
    ASSERT_TRUE(String("\"\\u005C\"") == "\\");
    ASSERT_TRUE(String("\"\\uD834\\uDD1E\"") == "𝄞");
    ASSERT_TRUE(String("\"𝄞\"") == "𝄞");
    ASSERT_TRUE(String("\"a\\b\"") == String("\"a\u005Cb\""));
    ASSERT_TRUE(String("\"Hello world!\"") == "Hello world!");
    ASSERT_TRUE(Number("42") == 42);
    ASSERT_TRUE(std::any_cast<bool>(ParseJSON("true")) == true);

    auto const image = ParseJSON(R"(
        {
            "Image": {
                "Width":  800,
                "Height": 600,
                "Title":  "View from 15th Floor",
                "Thumbnail": {
                    "Url":    "http://www.example.com/image/481989943",
                    "Height": 125,
                    "Width":  100
                },
                "Animated" : false,
                "IDs": [116, 943, 234, 38793]
            }
        }
    )");
    auto const& imageObject = std::any_cast<JSONObject const&>(std::any_cast<JSONObject const&>(image).at("Image"));
    ASSERT_TRUE(std::any_cast<double>(imageObject.at("Width")) == 800);
    ASSERT_TRUE(std::any_cast<std::string>(imageObject.at("Title")) == "View from 15th Floor");
    ASSERT_TRUE(std::any_cast<bool>(imageObject.at("Animated")) == false);
    ASSERT_TRUE(std::any_cast<JSONArray const&>(imageObject.at("IDs")).size() == 4);
    ASSERT_TRUE(std::any_cast<std::string>(std::any_cast<JSONObject const&>(imageObject.at("Thumbnail")).at("Url")) == "http://www.example.com/image/481989943");
    ASSERT_TRUE(StringifyJSON(image) == R"({"Image":{"Animated":false,"Height":600,"IDs":[116,943,234,38793],"Thumbnail":{"Height":125,"Url":"http://www.example.com/image/481989943","Width":100},"Title":"View from 15th Floor","Width":800}})");

    auto const places = ParseJSON(R"(
        [
            {
               "precision": "zip",
               "Latitude":  37.7668,
               "Longitude": -122.3959,
               "Address":   "",
               "City":      "SAN FRANCISCO",
               "State":     "CA",
               "Zip":       "94107",
               "Country":   "US"
            },
            {
               "precision": "zip",
               "Latitude":  37.371991,
               "Longitude": -122.026020,
               "Address":   "",
               "City":      "SUNNYVALE",
               "State":     "CA",
               "Zip":       "94085",
               "Country":   "US"
            }
        ])");
    auto const& placesArray = std::any_cast<JSONArray const&>(places);
    ASSERT_TRUE(placesArray.size() == 2);
    ASSERT_TRUE(std::any_cast<double>(std::any_cast<JSONObject const&>(placesArray[1]).at("Longitude")) == -122.026020);
    ASSERT_TRUE(StringifyJSON(ParseJSON(StringifyJSON(places))) == StringifyJSON(places));
}

/*
 * Ported from tests/src/unit-unicode2.cpp through unit-unicode5.cpp: an
 * exhaustive sweep of the RFC 3629 UTF-8 grammar, checking that every
 * well-formed sequence both parses and stringifies, and that every
 * ill-formed one is rejected by both. Well-formed sequences are swept in
 * full; for ill-formed ones the wrong byte is swept in full while the other
 * bytes take representative values, which keeps the run time reasonable.
 */

namespace {

void CheckUtf8(bool expected, int byte1, int byte2 = -1, int byte3 = -1, int byte4 = -1) {
    auto const bytes = Bytes(byte1, byte2, byte3, byte4);
    ASSERT_TRUE(Parses("\"" + bytes + "\"") == expected) << byte1 << " " << byte2 << " " << byte3 << " " << byte4;
    ASSERT_TRUE(Stringifies(bytes) == expected) << byte1 << " " << byte2 << " " << byte3 << " " << byte4;
    if (expected) {
        auto const text = StringifyJSON("abc" + bytes + "xyz");
        ASSERT_TRUE(text.substr(1, 3) == "abc");
        ASSERT_TRUE(text.substr(text.size() - 4, 3) == "xyz");
        ASSERT_TRUE(String(text) == "abc" + bytes + "xyz");
    } else {
        ASSERT_FALSE(Stringifies("abc" + bytes + "xyz"));
    }
}

bool Tail(int byte) {
    return byte >= 0x80 && byte <= 0xBF;
}

} /* namespace */

/* Bytes that can never start a sequence. */
TEST(JSONReference, Utf8IllFormedFirstByte) {
    for (auto byte1 = 0x80; byte1 <= 0xC1; ++byte1) {
        CheckUtf8(false, byte1);
    }
    for (auto byte1 = 0xF5; byte1 <= 0xFF; ++byte1) {
        CheckUtf8(false, byte1);
    }
}

/* UTF8-1 = %x00-7F. Control characters, the quote and the backslash need
 * escaping inside a JSON string. */
TEST(JSONReference, Utf8OneByte) {
    for (auto byte1 = 0x00; byte1 <= 0x7F; ++byte1) {
        auto const bytes = Bytes(byte1);
        if (byte1 <= 0x1F || byte1 == '"' || byte1 == '\\') {
            ASSERT_FALSE(Parses("\"" + bytes + "\"")) << byte1;
            /* Stringify escapes these rather than failing. */
            ASSERT_TRUE(String(StringifyJSON(bytes)) == bytes) << byte1;
        } else {
            CheckUtf8(true, byte1);
        }
    }
}

/* UTF8-2 = %xC2-DF UTF8-tail. */
TEST(JSONReference, Utf8TwoBytes) {
    for (auto byte1 = 0xC2; byte1 <= 0xDF; ++byte1) {
        CheckUtf8(false, byte1);
        for (auto byte2 = 0x00; byte2 <= 0xFF; ++byte2) {
            CheckUtf8(Tail(byte2), byte1, byte2);
        }
    }
}

/* UTF8-3 = %xE0 %xA0-BF UTF8-tail / %xE1-EC 2( UTF8-tail ) /
 *          %xED %x80-9F UTF8-tail / %xEE-EF 2( UTF8-tail ). */
TEST(JSONReference, Utf8ThreeBytes) {
    auto const sweep = [](int first, int last, int low2, int high2) {
        for (auto byte1 = first; byte1 <= last; ++byte1) {
            CheckUtf8(false, byte1);
            for (auto byte2 = low2; byte2 <= high2; ++byte2) {
                CheckUtf8(false, byte1, byte2);
                for (auto byte3 = 0x80; byte3 <= 0xBF; ++byte3) {
                    CheckUtf8(true, byte1, byte2, byte3);
                }
            }
            for (auto byte2 = 0x00; byte2 <= 0xFF; ++byte2) {
                if (byte2 >= low2 && byte2 <= high2) {
                    continue;
                }
                CheckUtf8(false, byte1, byte2);
                for (auto byte3 : {0x80, 0xA5, 0xBF}) {
                    CheckUtf8(false, byte1, byte2, byte3);
                }
            }
            for (auto byte2 : {low2, (low2 + high2) / 2, high2}) {
                for (auto byte3 = 0x00; byte3 <= 0xFF; ++byte3) {
                    if (Tail(byte3)) {
                        continue;
                    }
                    CheckUtf8(false, byte1, byte2, byte3);
                }
            }
        }
    };
    sweep(0xE0, 0xE0, 0xA0, 0xBF);
    sweep(0xE1, 0xEC, 0x80, 0xBF);
    sweep(0xED, 0xED, 0x80, 0x9F);
    sweep(0xEE, 0xEF, 0x80, 0xBF);
}

/* UTF8-4 = %xF0 %x90-BF 2( UTF8-tail ) / %xF1-F3 3( UTF8-tail ) /
 *          %xF4 %x80-8F 2( UTF8-tail ). */
TEST(JSONReference, Utf8FourBytes) {
    auto const sweep = [](int first, int last, int low2, int high2) {
        for (auto byte1 = first; byte1 <= last; ++byte1) {
            CheckUtf8(false, byte1);
            for (auto byte2 = low2; byte2 <= high2; ++byte2) {
                CheckUtf8(false, byte1, byte2);
                for (auto byte3 = 0x80; byte3 <= 0xBF; ++byte3) {
                    CheckUtf8(false, byte1, byte2, byte3);
                    for (auto byte4 = 0x80; byte4 <= 0xBF; ++byte4) {
                        CheckUtf8(true, byte1, byte2, byte3, byte4);
                    }
                }
            }
            for (auto byte2 = 0x00; byte2 <= 0xFF; ++byte2) {
                if (byte2 >= low2 && byte2 <= high2) {
                    continue;
                }
                for (auto byte3 : {0x80, 0xA5, 0xBF}) {
                    for (auto byte4 : {0x80, 0xA5, 0xBF}) {
                        CheckUtf8(false, byte1, byte2, byte3, byte4);
                    }
                }
            }
            for (auto byte2 : {low2, (low2 + high2) / 2, high2}) {
                for (auto byte3 = 0x00; byte3 <= 0xFF; ++byte3) {
                    if (Tail(byte3)) {
                        continue;
                    }
                    for (auto byte4 : {0x80, 0xA5, 0xBF}) {
                        CheckUtf8(false, byte1, byte2, byte3, byte4);
                    }
                }
                for (auto byte3 : {0x80, 0xA5, 0xBF}) {
                    for (auto byte4 = 0x00; byte4 <= 0xFF; ++byte4) {
                        if (Tail(byte4)) {
                            continue;
                        }
                        CheckUtf8(false, byte1, byte2, byte3, byte4);
                    }
                }
            }
        }
    };
    sweep(0xF0, 0xF0, 0x90, 0xBF);
    sweep(0xF1, 0xF3, 0x80, 0xBF);
    sweep(0xF4, 0xF4, 0x80, 0x8F);
}

/* Every \uXXXX escape decodes and re-encodes consistently, except lone
 * surrogates, which are rejected. */
TEST(JSONReference, UnicodeEscapesRoundTrip) {
    for (std::uint32_t codepoint = 0; codepoint <= 0xFFFF; ++codepoint) {
        char escape[8];
        std::snprintf(escape, sizeof(escape), "\\u%04X", codepoint);
        auto const text = std::string("\"") + escape + "\"";
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
            ASSERT_THROW(ParseJSON(text), JSONError) << text;
            continue;
        }
        auto const decoded = String(text);
        ASSERT_TRUE(String(StringifyJSON(decoded)) == decoded) << text;
    }
    /* All surrogate pairs decode to the supplementary planes and round-trip. */
    for (std::uint32_t high = 0xD800; high <= 0xDBFF; high += 0x11) {
        for (std::uint32_t low = 0xDC00; low <= 0xDFFF; low += 0x7) {
            char escape[16];
            std::snprintf(escape, sizeof(escape), "\\u%04X\\u%04X", high, low);
            auto const decoded = String(std::string("\"") + escape + "\"");
            ASSERT_TRUE(decoded.size() == 4) << escape;
            ASSERT_TRUE(String(StringifyJSON(decoded)) == decoded) << escape;
        }
    }
}

/*
 * Ported from tests/src/unit-serialization.cpp.
 */

/* Invalid UTF-8 in a value is an error when stringifying (the reference's
 * strict error handler). */
TEST(JSONReference, SerializationRejectsInvalidUtf8) {
    ASSERT_THROW(StringifyJSON(std::string("ä\xA9ü")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("123\xC2")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("123\xF1\xB0\x34\x35\x36")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("\xC2")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("\xC2\x41\x42")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("\xC2\xF4")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("\xF0\x80\x80\x41")), JSONError);
    ASSERT_THROW(StringifyJSON(std::string("\xE1\x80\xE2\xF0\x91\x92\xF1\xBF\x41")), JSONError);
    ASSERT_NO_THROW(StringifyJSON(std::string("äü")));
}

/* Simple values from the reference "exact dump string" table. Integral
 * values differ from the reference on purpose: it writes 1.0, we write 1. */
TEST(JSONReference, SerializationSimpleValues) {
    ASSERT_TRUE(StringifyJSON(0.5) == "0.5");
    ASSERT_TRUE(StringifyJSON(-0.5) == "-0.5");
    ASSERT_TRUE(StringifyJSON(1.5) == "1.5");
    ASSERT_TRUE(StringifyJSON(-2.25) == "-2.25");
    ASSERT_TRUE(StringifyJSON(0.0) == "0");
    ASSERT_TRUE(StringifyJSON(1.0) == "1");
    ASSERT_TRUE(StringifyJSON(-1.0) == "-1");
    ASSERT_TRUE(StringifyJSON(100.0) == "100");
    ASSERT_TRUE(StringifyJSON(std::numeric_limits<double>::quiet_NaN()) == "null");
    ASSERT_TRUE(StringifyJSON(std::numeric_limits<double>::infinity()) == "null");
    ASSERT_TRUE(StringifyJSON(-std::numeric_limits<double>::infinity()) == "null");
    ASSERT_TRUE(StringifyJSON(std::numeric_limits<long double>::quiet_NaN()) == "null");
    ASSERT_TRUE(StringifyJSON(std::numeric_limits<float>::infinity()) == "null");
}

/* Pretty printing matches the reference layout. */
TEST(JSONReference, SerializationPrettyPrinted) {
    auto const binary = JSONObject{{"bytes", JSONArray{1.0, 2.0, 3.0, 4.0}}, {"subtype", JSONNull{}}};
    ASSERT_TRUE(StringifyJSON(binary) == "{\"bytes\":[1,2,3,4],\"subtype\":null}");
    ASSERT_TRUE(StringifyJSON(binary, 4) ==
        "{\n"
        "    \"bytes\": [\n"
        "        1,\n"
        "        2,\n"
        "        3,\n"
        "        4\n"
        "    ],\n"
        "    \"subtype\": null\n"
        "}");
    auto const array = JSONArray{std::string("value"), 1.0, JSONObject{{"bytes", JSONArray{}}, {"subtype", 128.0}}};
    ASSERT_TRUE(StringifyJSON(array) == "[\"value\",1,{\"bytes\":[],\"subtype\":128}]");
    ASSERT_TRUE(StringifyJSON(array, 4) ==
        "[\n"
        "    \"value\",\n"
        "    1,\n"
        "    {\n"
        "        \"bytes\": [],\n"
        "        \"subtype\": 128\n"
        "    }\n"
        "]");
}

/*
 * Ported from tests/src/unit-to_chars.cpp.
 */

/* Formatting table for doubles. Where the reference writes a trailing ".0"
 * or switches to exponent form for integral values below 1e21, we write the
 * plain integer instead, as JavaScript does. */
TEST(JSONReference, ToCharsFormatting) {
    auto const check = [](double number, std::string const& expected) {
        ASSERT_TRUE(StringifyJSON(number) == expected) << number << " -> " << StringifyJSON(number) << " != " << expected;
    };
    check(-1.2345e-22, "-1.2345e-22");
    check(-1.2345e-21, "-1.2345e-21");
    check(-1.2345e-20, "-1.2345e-20");
    check(-1.2345e-19, "-1.2345e-19");
    check(-1.2345e-18, "-1.2345e-18");
    check(-1.2345e-17, "-1.2345e-17");
    check(-1.2345e-16, "-1.2345e-16");
    check(-1.2345e-15, "-1.2345e-15");
    check(-1.2345e-14, "-1.2345e-14");
    check(-1.2345e-13, "-1.2345e-13");
    check(-1.2345e-12, "-1.2345e-12");
    check(-1.2345e-11, "-1.2345e-11");
    check(-1.2345e-10, "-1.2345e-10");
    check(-1.2345e-9, "-1.2345e-09");
    check(-1.2345e-8, "-1.2345e-08");
    check(-1.2345e-7, "-1.2345e-07");
    check(-1.2345e-6, "-1.2345e-06");
    check(-1.2345e-5, "-1.2345e-05");
    check(-1.2345e-4, "-0.00012345");
    check(-1.2345e-3, "-0.0012345");
    check(-1.2345e-2, "-0.012345");
    check(-1.2345e-1, "-0.12345");
    check(-0.0, "0");
    check(0.0, "0");
    check(1.2345e+0, "1.2345");
    check(1.2345e+1, "12.345");
    check(1.2345e+2, "123.45");
    check(1.2345e+3, "1234.5");
    check(1.2345e+4, "12345");
    check(1.2345e+5, "123450");
    check(1.2345e+6, "1234500");
    check(1.2345e+7, "12345000");
    check(1.2345e+8, "123450000");
    check(1.2345e+9, "1234500000");
    check(1.2345e+10, "12345000000");
    check(1.2345e+11, "123450000000");
    check(1.2345e+12, "1234500000000");
    check(1.2345e+13, "12345000000000");
    check(1.2345e+14, "123450000000000");
    check(1.2345e+15, "1234500000000000");
    check(1.2345e+16, "12345000000000000");
    check(1.2345e+17, "123450000000000000");
    check(1.2345e+18, "1234500000000000000");
    check(1.2345e+19, "12345000000000000000");
    check(1.2345e+20, "123450000000000000000");
    check(1.2345e+21, "1.2345e+21");
    check(1.2345e+22, "1.2345e+22");
}

/* The reference's Grisu digit-generation table, as round-trip checks: the
 * shortest representation must parse back to the same double and must not
 * need more digits than the reference produces. */
TEST(JSONReference, ToCharsDigitGeneration) {
    auto const makeDouble = [](std::uint64_t sign, std::uint64_t exponent, std::uint64_t mantissa) {
        auto const bits = (sign << 63) | (exponent << 52) | mantissa;
        double number;
        std::memcpy(&number, &bits, sizeof(number));
        return number;
    };
    auto const check = [](double number, std::string const& digits) {
        auto const text = StringifyJSON(number);
        ASSERT_TRUE(Number(text) == number) << text;
        std::size_t significant = 0;
        bool leading = true;
        for (std::size_t i = 0; i < text.size() && text[i] != 'e'; ++i) {
            auto const c = text[i];
            if (c < '0' || c > '9') {
                continue;
            }
            if (leading && c == '0') {
                continue;
            }
            leading = false;
            ++significant;
        }
        /* Integral values below 1e21 are written out in full rather than
         * in shortest form, so only their round trip is checked. */
        if (text.find('.') == std::string::npos && text.find('e') == std::string::npos) {
            return;
        }
        ASSERT_TRUE(significant <= digits.size()) << text << " vs " << digits;
    };
    check(makeDouble(0, 0, 0x0000000000000001), "5");
    check(makeDouble(0, 0, 0x000FFFFFFFFFFFFF), "2225073858507201");
    check(makeDouble(0, 1, 0x0000000000000000), "22250738585072014");
    check(makeDouble(0, 1, 0x0000000000000001), "2225073858507202");
    check(makeDouble(0, 1, 0x000FFFFFFFFFFFFF), "44501477170144023");
    check(makeDouble(0, 2, 0x0000000000000000), "4450147717014403");
    check(makeDouble(0, 2, 0x0000000000000001), "4450147717014404");
    check(makeDouble(0, 4, 0x0000000000000000), "17800590868057611");
    check(makeDouble(0, 5, 0x0000000000000000), "35601181736115222");
    check(makeDouble(0, 6, 0x0000000000000000), "7120236347223045");
    check(makeDouble(0, 10, 0x0000000000000000), "11392378155556871");
    check(makeDouble(0, 2046, 0x000FFFFFFFFFFFFE), "17976931348623155");
    check(makeDouble(0, 2046, 0x000FFFFFFFFFFFFF), "17976931348623157");
    check(10000, "1");
    check(1200000, "12");
    check(4.9406564584124654e-324, "5");
    check(2.2250738585072009e-308, "2225073858507201");
    check(1.82877982605164e-99, "182877982605164");
    check(1.1505466208671903e-09, "11505466208671903");
    check(5.5645893133766722e+20, "5564589313376672");
    check(53.034830388866226, "53034830388866226");
    check(0.0021066531670178605, "21066531670178605");
    check(9.5e-088, "95");
    check(4.65e-233, "465");
    check(1.415e+087, "1415");
    check(3.9815e+037, "39815");
    check(4.10405e+045, "410405");
    check(2.920845e+234, "2920845");
    check(2.8919465e-122, "28919465");
    check(4.37877185e-303, "437877185");
    check(1.227701635e+129, "1227701635");
    check(1.8415524525e+129, "18415524525");
    check(5.48357443505e+043, "548357443505");
    check(3.891901811465e+229, "3891901811465");
    check(1.9459509057325e+229, "19459509057325");
    check(1.44609583816055e+051, "144609583816055");
    check(4.173677474585315e-286, "4173677474585315");
    check(1.1079507728788885e-197, "11079507728788885");
    check(1.234550136632744e-099, "1234550136632744");
    check(9.2503171196036502e+232, "925031711960365");
    check(4.1980471502848898e-234, "419804715028489");
    check(1.1716315319786511e-088, "11716315319786511");
    check(4.3281007284461249e+076, "4328100728446125");
    check(3.3177101181600311e-127, "3317710118160031");
    check(2.5e+302, "25");
    check(7.55e+176, "755");
    check(3.775e+176, "3775");
    check(4.3495e-273, "43495");
    check(2.30365e-028, "230365");
    check(1.263005e+125, "1263005");
    check(7.1422105e-036, "71422105");
    check(1.39345735e-241, "139345735");
    check(1.414634485e-219, "1414634485");
    check(4.5392779195e-100, "45392779195");
    check(2.26963895975e-100, "226963895975");
    check(1.134819479875e-100, "1134819479875");
    check(7.7003665618895e-060, "77003665618895");
    check(3.85018328094475e-060, "385018328094475");
    check(1.925091640472375e-060, "1925091640472375");
    check(6.8985865317742005e+180, "68985865317742005");
    check(1.3076622631878654e+065, "13076622631878654");
    check(1.3605202075612124e+216, "13605202075612124");
    check(3.5928102174759597e+223, "35928102174759597");
    check(8.9125197712484552e+192, "8912519771248455");
    check(5.5876975736230114e+097, "55876975736230114");
    check(1.1762578307285404e-119, "11762578307285404");
    check(8.4999999999999993e-036, "8499999999999999");
    check(2.5499999999999999e-035, "255");
    check(2.0049999999999997e-014, "20049999999999997");
    check(7.9844999999999994e-017, "7984499999999999");
    check(1.1165499999999999e-035, "11165499999999999");
    check(1.581615e-025, "1581615");
    check(1.3809855e-017, "13809855");
    check(1.2046404499999999e-026, "12046404499999999");
    check(9.3251405449999991e-030, "9325140544999999");
    check(1.2092014595e-017, "12092014595");
    check(3.2007045838499998e-007, "320070458385");
    check(8.391946324354999e-015, "8391946324354999");
    check(1.2253990460585e-017, "12253990460585");
    check(6.8735641489760495e-014, "687356414897605");
    check(7.459816430480385e-032, "7459816430480385");
    check(2.7960588398142552e-034, "2796058839814255");
    check(1.3980294199071276e-034, "13980294199071276");
    check(6.6279012373057359e-011, "6627901237305736");
    check(6.9177880043968072e-024, "6917788004396807");
    check(3.7915693108349708e-012, "3791569310834971");
    check(3.4080817676591365e-015, "34080817676591365");
    check(1.7040408838295683e-015, "17040408838295683");
    check(2.4999999999999998e-012, "25");
    check(1.2499999999999999e-012, "125");
    check(4.9949999999999996e-024, "49949999999999996");
    check(3.9304999999999998e-031, "39304999999999998");
    check(5.2770499999999992e-038, "5277049999999999");
    check(1.223955e-033, "1223955");
    check(1.8799584999999998e+034, "18799584999999998");
    check(2.01387715e-025, "201387715");
    check(5.8490641049999989e-014, "5849064104999999");
    check(5.9349003054999999e-037, "59349003055");
    check(1.2284718039499998e-027, "12284718039499998");
    check(8.9270767180849991e-035, "8927076718084999");
    check(4.4635383590424995e-035, "44635383590424995");
    check(1.0016990862549499e-019, "10016990862549499");
    check(5.4829412628024647e-036, "5482941262802465");
    check(9.5290783281036439e-017, "9529078328103644");
    check(8.9662279366405553e-028, "8966227936640555");
    check(8.3086234418058538e-029, "8308623441805854");
    check(6.1985873566126555e-027, "61985873566126555");
    check(3.0992936783063277e-027, "30992936783063277");
    check(6.9123512506176015e-030, "6912351250617602");
    check(3.4561756253088008e-030, "3456175625308801");
}

/* Large integers stay exact as long as they fit a double. */
TEST(JSONReference, ToCharsIntegers) {
    ASSERT_TRUE(StringifyJSON(-9223372036854775808.0) == "-9223372036854775808");
    ASSERT_TRUE(StringifyJSON(9007199254740991.0) == "9007199254740991");
    ASSERT_TRUE(StringifyJSON(-9007199254740991.0) == "-9007199254740991");
    ASSERT_TRUE(StringifyJSON(4294967295.0) == "4294967295");
    ASSERT_TRUE(StringifyJSON(-2147483648.0) == "-2147483648");
    ASSERT_TRUE(Number(StringifyJSON(-3456789012345678901.0)) == -3456789012345678901.0);
}
