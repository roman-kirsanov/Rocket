/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <charconv>
#include <format>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/JSON.hpp>

namespace Rocket {

static std::size_t _Utf8SequenceLength(std::string const& text, std::size_t offset) {
    PROFILE

    auto const size = text.size();
    auto const byte = [&](std::size_t index) -> int {
        return offset + index < size ? static_cast<unsigned char>(text[offset + index]) : -1;
    };
    auto const in = [](int value, int low, int high) {
        return value >= low && value <= high;
    };

    auto const first = byte(0);
    if (first < 0x80) {
        return 1;
    }
    if (in(first, 0xC2, 0xDF)) {
        return in(byte(1), 0x80, 0xBF) ? 2 : 0;
    }
    if (first == 0xE0) {
        return in(byte(1), 0xA0, 0xBF) && in(byte(2), 0x80, 0xBF) ? 3 : 0;
    }
    if (in(first, 0xE1, 0xEC) || in(first, 0xEE, 0xEF)) {
        return in(byte(1), 0x80, 0xBF) && in(byte(2), 0x80, 0xBF) ? 3 : 0;
    }
    if (first == 0xED) {
        return in(byte(1), 0x80, 0x9F) && in(byte(2), 0x80, 0xBF) ? 3 : 0;
    }
    if (first == 0xF0) {
        return in(byte(1), 0x90, 0xBF) && in(byte(2), 0x80, 0xBF) && in(byte(3), 0x80, 0xBF) ? 4 : 0;
    }
    if (in(first, 0xF1, 0xF3)) {
        return in(byte(1), 0x80, 0xBF) && in(byte(2), 0x80, 0xBF) && in(byte(3), 0x80, 0xBF) ? 4 : 0;
    }
    if (first == 0xF4) {
        return in(byte(1), 0x80, 0x8F) && in(byte(2), 0x80, 0xBF) && in(byte(3), 0x80, 0xBF) ? 4 : 0;
    }
    return 0;
}

std::string StringifyJSON(JSONValue const& value, int indent) {
    PROFILE

    std::size_t const step = (indent > 0 ? indent : 0);
    std::string out = {};

    auto const newline = [&](int level) -> void {
        if (step > 0) {
            out += '\n';
            out.append(static_cast<std::size_t>(level) * step, ' ');
        }
    };

    auto const writeNumber = [&](double number) -> void {
        if (!std::isfinite(number)) {
            out += "null";
            return;
        }
        if (number == 0) {
            out += '0'; /* also for negative zero */
            return;
        }
        char buffer[400];
        auto result = std::to_chars_result{};
        /* Integral values print as plain integers (up to the point where the
         * digits stop being meaningful, like JavaScript's 1e21 cutoff);
         * everything else uses the shortest form that round-trips. */
        if (number == std::floor(number) && std::fabs(number) < 1e21) {
            result = std::to_chars(buffer, buffer + sizeof(buffer), number, std::chars_format::fixed);
        } else {
            result = std::to_chars(buffer, buffer + sizeof(buffer), number);
        }
        out.append(buffer, result.ptr);
    };

    auto const writeString = [&](std::string const& string) -> void {
        static constexpr auto Hex = "0123456789abcdef";
        out += '"';
        auto position = 0uz;
        while (position < string.size()) {
            auto const c = static_cast<unsigned char>(string[position]);
            switch (c) {
                case '"':  out += "\\\""; ++position; continue;
                case '\\': out += "\\\\"; ++position; continue;
                case '\b': out += "\\b";  ++position; continue;
                case '\f': out += "\\f";  ++position; continue;
                case '\n': out += "\\n";  ++position; continue;
                case '\r': out += "\\r";  ++position; continue;
                case '\t': out += "\\t";  ++position; continue;
                default:
                    break;
            }
            if (c < 0x20) {
                out += "\\u00";
                out += Hex[c >> 4];
                out += Hex[c & 0xF];
                ++position;
                continue;
            }
            auto const length = _Utf8SequenceLength(string, position);
            if (length == 0) {
                throw JSONError(std::format("cannot stringify string with invalid UTF-8 at byte {}", position));
            }
            out.append(string, position, length);
            position += length;
        }
        out += '"';
    };

    auto const writeArray = [&](JSONArray const& array, int level, auto const& write) -> void {
        if (array.empty()) {
            out += "[]";
            return;
        }
        out += '[';
        auto first = true;
        for (auto const& element : array) {
            if (!first) {
                out += ',';
            }
            first = false;
            newline(level + 1);
            write(element, level + 1);
        }
        newline(level);
        out += ']';
    };

    auto const writeObject = [&](JSONObject const& object, int level, auto const& write) -> void {
        if (object.empty()) {
            out += "{}";
            return;
        }
        out += '{';
        auto first = true;
        for (auto const& [key, element] : object) {
            if (!first) {
                out += ',';
            }
            first = false;
            newline(level + 1);
            writeString(key);
            out += step > 0 ? ": " : ":";
            write(element, level + 1);
        }
        newline(level);
        out += '}';
    };

    auto const writeNumberAs = [&]<typename... Types>(JSONValue const& any) -> bool {
        return ((any.type() == typeid(Types) && (writeNumber(static_cast<double>(std::any_cast<Types>(any))), true)) || ...);
    };

    auto const write = [&](this auto const& self, JSONValue const& any, int level) -> void {
        auto const& type = any.type();
        if (!any.has_value() || type == typeid(JSONNull) || type == typeid(std::nullptr_t)) {
            out += "null";
        } else if (type == typeid(bool)) {
            out += std::any_cast<bool>(any) ? "true" : "false";
        } else if (type == typeid(double)) {
            writeNumber(std::any_cast<double>(any));
        } else if (type == typeid(std::string)) {
            writeString(std::any_cast<std::string const&>(any));
        } else if (type == typeid(JSONArray)) {
            writeArray(std::any_cast<JSONArray const&>(any), level, self);
        } else if (type == typeid(JSONObject)) {
            writeObject(std::any_cast<JSONObject const&>(any), level, self);
        } else if (type == typeid(char const*)) {
            writeString(std::any_cast<char const*>(any));
        } else if (type == typeid(char*)) {
            writeString(std::any_cast<char*>(any));
        } else if (!writeNumberAs.template operator()<
                int, unsigned int, long, unsigned long, long long, unsigned long long,
                short, unsigned short, signed char, unsigned char, char, float, long double>(any)) {
            throw JSONError(std::format("cannot stringify a value of type '{}' as JSON", type.name()));
        }
    };

    write(value, 0);

    return out;
}

JSONValue ParseJSON(std::string const& text) {
    PROFILE

    auto const size = text.size();
    auto position = 0uz;
    auto depth = 0;

    auto const failAt = [&](std::size_t at, std::string const& message) -> void {
        auto line = 1uz;
        auto start = 0uz;
        for (auto i = 0uz; i < at && i < size; ++i) {
            if (text[i] == '\n') {
                ++line;
                start = i + 1;
            }
        }
        throw JSONError(std::format("JSON parse error at line {}, column {}: {}", line, at - start + 1, message));
    };

    auto const fail = [&](std::string const& message) -> void {
        failAt(position, message);
    };

    auto const peek = [&]() -> int {
        return position < size ? static_cast<unsigned char>(text[position]) : -1;
    };

    auto const isDigit = [&]() -> bool {
        auto const c = peek();
        return c >= '0' && c <= '9';
    };

    auto const describe = [&]() -> std::string {
        auto const c = peek();
        if (c < 0) {
            return "end of input";
        }
        if (c >= 0x20 && c < 0x7F) {
            return std::format("'{}'", static_cast<char>(c));
        }
        return std::format("byte 0x{:02X}", c);
    };

    auto const skipWhitespace = [&]() -> void {
        while (true) {
            auto const c = peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++position;
            } else {
                return;
            }
        }
    };

    auto const parseLiteral = [&](char const* word) -> void {
        auto const start = position;
        for (auto p = word; *p != '\0'; ++p, ++position) {
            if (peek() != static_cast<unsigned char>(*p)) {
                failAt(start, std::format("invalid literal, expected '{}'", word));
            }
        }
    };

    auto const parseHex4 = [&]() -> std::uint32_t {
        auto value = 0u;
        for (auto i = 0; i < 4; ++i, ++position) {
            auto const c = peek();
            auto digit = 0;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                digit = c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                digit = c - 'A' + 10;
            } else {
                fail("'\\u' must be followed by 4 hex digits");
            }
            value = (value << 4) | static_cast<std::uint32_t>(digit);
        }
        return value;
    };

    auto const appendCodepoint = [](std::string& out, std::uint32_t codepoint) -> void {
        if (codepoint < 0x80) {
            out += static_cast<char>(codepoint);
        } else if (codepoint < 0x800) {
            out += static_cast<char>(0xC0 | (codepoint >> 6));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            out += static_cast<char>(0xE0 | (codepoint >> 12));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (codepoint >> 18));
            out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    };

    auto const parseEscape = [&](std::string& result) -> void {
        auto const start = position;
        ++position;
        auto const c = peek();
        ++position;
        switch (c) {
            case '"':  result += '"';  return;
            case '\\': result += '\\'; return;
            case '/':  result += '/';  return;
            case 'b':  result += '\b'; return;
            case 'f':  result += '\f'; return;
            case 'n':  result += '\n'; return;
            case 'r':  result += '\r'; return;
            case 't':  result += '\t'; return;
            case 'u': {
                auto codepoint = parseHex4();
                if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                    /* High surrogate; a low surrogate escape must follow. */
                    if (peek() != '\\' || position + 1 >= size || text[position + 1] != 'u') {
                        failAt(start, "high surrogate must be followed by a '\\u' low surrogate escape");
                    }
                    position += 2;
                    auto const low = parseHex4();
                    if (low < 0xDC00 || low > 0xDFFF) {
                        failAt(start, "high surrogate must be followed by a low surrogate (U+DC00..U+DFFF)");
                    }
                    codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                    failAt(start, "low surrogate (U+DC00..U+DFFF) must follow a high surrogate");
                }
                appendCodepoint(result, codepoint);
                return;
            }
            default:
                position = start + 1;
                if (c < 0) {
                    failAt(start, "unexpected end of input in escape sequence");
                }
                failAt(start, std::format("invalid escape sequence '\\{}'", describe()));
        }
    };

    auto const parseString = [&]() -> JSONString {
        auto const start = position;
        ++position;
        auto result = std::string{};
        while (true) {
            auto const c = peek();
            if (c < 0) {
                failAt(start, "unterminated string");
            }
            if (c == '"') {
                ++position;
                return result;
            }
            if (c == '\\') {
                parseEscape(result);
                continue;
            }
            if (c < 0x20) {
                fail(std::format("control character (0x{:02X}) in string must be escaped", c));
            }
            auto const length = _Utf8SequenceLength(text, position);
            if (length == 0) {
                fail("invalid UTF-8 in string");
            }
            result.append(text, position, length);
            position += length;
        }
    };

    auto const parseNumber = [&]() -> JSONNumber {
        auto const start = position;
        if (peek() == '-') {
            ++position;
        }
        if (peek() == '0') {
            ++position;
        } else if (isDigit()) {
            while (isDigit()) {
                ++position;
            }
        } else {
            fail(std::format("unexpected {}, expected a digit", describe()));
        }
        if (peek() == '.') {
            ++position;
            if (!isDigit()) {
                fail(std::format("unexpected {}, expected a digit after the decimal point", describe()));
            }
            while (isDigit()) {
                ++position;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            ++position;
            if (peek() == '+' || peek() == '-') {
                ++position;
            }
            if (!isDigit()) {
                fail(std::format("unexpected {}, expected a digit in the exponent", describe()));
            }
            while (isDigit()) {
                ++position;
            }
        }

        auto const first = text.data() + start;
        auto const last = text.data() + position;
        auto value = 0.0;
        auto const result = std::from_chars(first, last, value);
        if (result.ec == std::errc::result_out_of_range) {
            /* Values too small for a double quietly underflow to zero, but
             * overflow to infinity is an error, as in the reference
             * implementation. from_chars leaves `value` unspecified here, so
             * let strtod (locale-independent for this token) decide which. */
            value = std::strtod(std::string(first, last).c_str(), nullptr);
            if (std::isinf(value)) {
                failAt(start, std::format("number overflow parsing '{}'", std::string_view(first, last)));
            }
        } else if (result.ec != std::errc{} || result.ptr != last) {
            failAt(start, "invalid number");
        }
        return value;
    };

    auto const enter = [&]() -> void {
        auto constexpr _MaxDepth = 1024;
        if (++depth > _MaxDepth) {
            fail(std::format("nesting deeper than {} levels", _MaxDepth));
        }
    };

    auto const parseArray = [&](auto const& parseValue) -> JSONArray {
        enter();
        ++position;
        auto array = JSONArray{};
        skipWhitespace();
        if (peek() == ']') {
            ++position;
            --depth;
            return array;
        }
        while (true) {
            array.push_back(parseValue());
            skipWhitespace();
            if (peek() == ',') {
                ++position;
                continue;
            }
            if (peek() == ']') {
                ++position;
                --depth;
                return array;
            }
            fail(std::format("unexpected {}, expected ',' or ']' in array", describe()));
        }
    };

    auto const parseObject = [&](auto const& parseValue) -> JSONObject {
        enter();
        ++position;
        auto object = JSONObject{};
        skipWhitespace();
        if (peek() == '}') {
            ++position;
            --depth;
            return object;
        }
        while (true) {
            skipWhitespace();
            if (peek() != '"') {
                fail(std::format("unexpected {}, expected a string key", describe()));
            }
            auto key = parseString();
            skipWhitespace();
            if (peek() != ':') {
                fail(std::format("unexpected {}, expected ':' after object key", describe()));
            }
            ++position;
            auto value = parseValue();
            object.insert_or_assign(std::move(key), std::move(value));
            skipWhitespace();
            if (peek() == ',') {
                ++position;
                continue;
            }
            if (peek() == '}') {
                ++position;
                --depth;
                return object;
            }
            fail(std::format("unexpected {}, expected ',' or '}}' in object", describe()));
        }
    };

    auto const parseValue = [&](this auto const& self) -> JSONValue {
        skipWhitespace();
        switch (peek()) {
            case '{':
                return parseObject(self);
            case '[':
                return parseArray(self);
            case '"':
                return parseString();
            case 't':
                parseLiteral("true");
                return true;
            case 'f':
                parseLiteral("false");
                return false;
            case 'n':
                parseLiteral("null");
                return JSONNull{};
            case '-':
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                return parseNumber();
            case -1:
                fail("unexpected end of input, expected a value");
            default:
                fail(std::format("unexpected character {}, expected a value", describe()));
        }
        return {};
    };

    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        position = 3;
    }

    auto value = parseValue();

    skipWhitespace();

    if (position < size) {
        fail(std::format("unexpected trailing character {}", describe()));
    }

    return value;
}

bool IsJSONNull(JSONValue const& value) {
    PROFILE

    return !value.has_value() || value.type() == typeid(JSONNull) || value.type() == typeid(std::nullptr_t);
}

bool IsJSONBoolean(JSONValue const& value) {
    PROFILE

    return value.type() == typeid(JSONBoolean);
}

bool IsJSONNumber(JSONValue const& value) {
    PROFILE

    return value.type() == typeid(JSONNumber);
}

bool IsJSONString(JSONValue const& value) {
    PROFILE

    return value.type() == typeid(JSONString);
}

bool IsJSONArray(JSONValue const& value) {
    PROFILE

    return value.type() == typeid(JSONArray);
}

bool IsJSONObject(JSONValue const& value) {
    PROFILE

    return value.type() == typeid(JSONObject);
}

/* Names the JSON type held by a value for error messages, falling back to the
 * C++ type name for anything that is not a canonical JSON type. */
static std::string _DescribeJSONType(JSONValue const& value) {
    PROFILE

    if (IsJSONNull(value)) return "null";
    if (IsJSONBoolean(value)) return "a boolean";
    if (IsJSONNumber(value)) return "a number";
    if (IsJSONString(value)) return "a string";
    if (IsJSONArray(value)) return "an array";
    if (IsJSONObject(value)) return "an object";
    return std::format("a value of C++ type '{}'", value.type().name());
}

template<typename Type>
static Type& _AsJSON(JSONValue& value, char const* expected) {
    PROFILE

    if (value.type() != typeid(Type)) {
        throw JSONError(std::format("expected {}, got {}", expected, _DescribeJSONType(value)));
    }
    return *std::any_cast<Type>(&value);
}

template<typename Type>
static Type const& _AsJSON(JSONValue const& value, char const* expected) {
    PROFILE

    if (value.type() != typeid(Type)) {
        throw JSONError(std::format("expected {}, got {}", expected, _DescribeJSONType(value)));
    }
    return *std::any_cast<Type>(&value);
}

JSONBoolean AsJSONBoolean(JSONValue const& value) {
    PROFILE

    return _AsJSON<JSONBoolean>(value, "a JSON boolean");
}

JSONNumber AsJSONNumber(JSONValue const& value) {
    PROFILE

    return _AsJSON<JSONNumber>(value, "a JSON number");
}

JSONString const& AsJSONString(JSONValue const& value) {
    PROFILE

    return _AsJSON<JSONString>(value, "a JSON string");
}

JSONString& AsJSONString(JSONValue& value) {
    PROFILE

    return _AsJSON<JSONString>(value, "a JSON string");
}

JSONArray const& AsJSONArray(JSONValue const& value) {
    PROFILE

    return _AsJSON<JSONArray>(value, "a JSON array");
}

JSONArray& AsJSONArray(JSONValue& value) {
    PROFILE

    return _AsJSON<JSONArray>(value, "a JSON array");
}

JSONObject const& AsJSONObject(JSONValue const& value) {
    PROFILE

    return _AsJSON<JSONObject>(value, "a JSON object");
}

JSONObject& AsJSONObject(JSONValue& value) {
    PROFILE

    return _AsJSON<JSONObject>(value, "a JSON object");
}

} /* namespace Rocket */
