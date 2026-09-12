/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <Rocket/Base/Enum.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* A default-constructed Enum holds the first alternative. */
TEST(Enum, DefaultConstructedHoldsFirstAlternative) {
    Enum<int, std::string, double> value;
    ASSERT_TRUE(value.is<int>());
    ASSERT_TRUE(!value.is<std::string>());
    ASSERT_TRUE(!value.is<double>());
    ASSERT_TRUE(value.as<int>() != nullptr);
    ASSERT_TRUE(*value.as<int>() == 0);
}

/* is() reports exactly the active alternative. */
TEST(Enum, IsReportsActiveAlternative) {
    Enum<int, std::string, double> value = std::string("hello");
    ASSERT_TRUE(value.is<std::string>());
    ASSERT_TRUE(!value.is<int>());
    ASSERT_TRUE(!value.is<double>());
}

/* as() returns a pointer to the value or nullptr for inactive types. */
TEST(Enum, AsReturnsPointerOrNullptr) {
    Enum<int, std::string, double> value = 42;
    ASSERT_TRUE(value.as<int>() != nullptr);
    ASSERT_TRUE(*value.as<int>() == 42);
    ASSERT_TRUE(value.as<std::string>() == nullptr);
    ASSERT_TRUE(value.as<double>() == nullptr);
}

/* The mutable as() pointer writes through to the stored value. */
TEST(Enum, MutableAsWritesThrough) {
    Enum<int, std::string, double> value = 1;
    *value.as<int>() = 99;
    ASSERT_TRUE(*value.as<int>() == 99);
}

/* The const as() overload returns a const pointer to the value. */
TEST(Enum, ConstAsReturnsConstPointer) {
    Enum<int, std::string, double> const value = std::string("abc");
    std::string const* stored = value.as<std::string>();
    ASSERT_TRUE(stored != nullptr);
    ASSERT_TRUE(*stored == "abc");
    ASSERT_TRUE(value.as<int>() == nullptr);
    ASSERT_TRUE(value.is<std::string>());
}

/* Assignment switches the active alternative. */
TEST(Enum, AssignmentSwitchesAlternative) {
    Enum<int, std::string, double> value = 7;
    ASSERT_TRUE(value.is<int>());

    value = std::string("switched");
    ASSERT_TRUE(value.is<std::string>());
    ASSERT_TRUE(*value.as<std::string>() == "switched");
    ASSERT_TRUE(value.as<int>() == nullptr);

    value = 3.5;
    ASSERT_TRUE(value.is<double>());
    ASSERT_TRUE(*value.as<double>() == 3.5);

    value = 11;
    ASSERT_TRUE(value.is<int>());
    ASSERT_TRUE(*value.as<int>() == 11);
}

/* match() dispatches to the visitor for the active alternative. */
TEST(Enum, MatchDispatchesToActiveVisitor) {
    Enum<int, std::string, double> value = 21;
    auto result = value.match(
        [](int v) { return v * 2; },
        [](std::string const& s) { return (int)s.size(); },
        [](double d) { return (int)d; }
    );
    ASSERT_TRUE(result == 42);

    value = std::string("four");
    result = value.match(
        [](int v) { return v * 2; },
        [](std::string const& s) { return (int)s.size(); },
        [](double d) { return (int)d; }
    );
    ASSERT_TRUE(result == 4);

    value = 2.75;
    result = value.match(
        [](int v) { return v * 2; },
        [](std::string const& s) { return (int)s.size(); },
        [](double d) { return (int)d; }
    );
    ASSERT_TRUE(result == 2);
}

/* One visitor may cover several alternatives via a generic lambda. */
TEST(Enum, GenericLambdaCoversSeveralAlternatives) {
    Enum<int, std::string, double> value = std::string("text");
    auto result = value.match(
        [](std::string const&) { return 1; },
        [](auto const&) { return 0; }
    );
    ASSERT_TRUE(result == 1);

    value = 5;
    result = value.match(
        [](std::string const&) { return 1; },
        [](auto const&) { return 0; }
    );
    ASSERT_TRUE(result == 0);
}

/* The const match() overload dispatches identically. */
TEST(Enum, ConstMatchDispatchesIdentically) {
    Enum<int, std::string, double> const value = 3.25;
    auto const result = value.match(
        [](int) { return std::string("int"); },
        [](std::string const&) { return std::string("string"); },
        [](double) { return std::string("double"); }
    );
    ASSERT_TRUE(result == "double");
}

/* A mutable match() visitor can modify the stored value in place. */
TEST(Enum, MutableMatchVisitorModifiesInPlace) {
    Enum<int, std::string> value = std::string("ab");
    value.match(
        [](int& v) { v += 1; },
        [](std::string& s) { s += "c"; }
    );
    ASSERT_TRUE(*value.as<std::string>() == "abc");
}
