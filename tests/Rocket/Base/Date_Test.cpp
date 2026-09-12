/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Date.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* The Unix epoch has all-zero components and a Thursday weekday. */
TEST(Date, UnixEpochComponents) {
    auto const epoch = Date(0);
    ASSERT_TRUE(epoch.toMilliseconds() == 0);
    ASSERT_TRUE(epoch.getYear() == 1970);
    ASSERT_TRUE(epoch.getMonth() == 1);
    ASSERT_TRUE(epoch.getDay() == 1);
    ASSERT_TRUE(epoch.getHour() == 0);
    ASSERT_TRUE(epoch.getMinute() == 0);
    ASSERT_TRUE(epoch.getSecond() == 0);
    ASSERT_TRUE(epoch.getMillisecond() == 0);
    ASSERT_TRUE(epoch.getWeekday() == 4);
    ASSERT_TRUE(epoch.getOrdinal() == 1);
    ASSERT_TRUE(epoch.getQuarter() == 1);
}

/* The components constructor round-trips through the calendar accessors. */
TEST(Date, ComponentsConstructorRoundTrips) {
    auto const date = Date(2024, 1, 15, 10, 30, 45, 123);
    ASSERT_TRUE(date.getYear() == 2024);
    ASSERT_TRUE(date.getMonth() == 1);
    ASSERT_TRUE(date.getDay() == 15);
    ASSERT_TRUE(date.getHour() == 10);
    ASSERT_TRUE(date.getMinute() == 30);
    ASSERT_TRUE(date.getSecond() == 45);
    ASSERT_TRUE(date.getMillisecond() == 123);
    ASSERT_TRUE(date.getWeekday() == 1); /* 2024-01-15 is a Monday */
    ASSERT_TRUE(date.getOrdinal() == 15);
    ASSERT_TRUE(date.getQuarter() == 1);
}

/* The components and milliseconds constructors agree on the same instant. */
TEST(Date, ConstructorsAgreeOnSameInstant) {
    auto const a = Date(2024, 1, 15, 10, 30, 45, 123);
    auto const b = Date(a.toMilliseconds());
    ASSERT_TRUE(a == b);
}

/* Comparison operators order dates by instant. */
TEST(Date, ComparisonOperatorsOrderByInstant) {
    auto const earlier = Date(2024, 1, 1);
    auto const later = Date(2024, 1, 2);
    ASSERT_TRUE(earlier < later);
    ASSERT_TRUE(later > earlier);
    ASSERT_TRUE(earlier <= later);
    ASSERT_TRUE(earlier <= earlier);
    ASSERT_TRUE(later >= earlier);
    ASSERT_TRUE(later >= later);
    ASSERT_TRUE(earlier != later);
    ASSERT_TRUE(earlier == earlier);
    ASSERT_TRUE(!(earlier == later));
}

/* Quarters map from months as expected. */
TEST(Date, QuartersMapFromMonths) {
    ASSERT_TRUE(Date(2024, 3, 31).getQuarter() == 1);
    ASSERT_TRUE(Date(2024, 4, 1).getQuarter() == 2);
    ASSERT_TRUE(Date(2024, 9, 30).getQuarter() == 3);
    ASSERT_TRUE(Date(2024, 12, 31).getQuarter() == 4);
}

/* Leap-year logic covers century rules and February lengths. */
TEST(Date, LeapYearLogic) {
    ASSERT_TRUE(Date(2024, 6, 1).isLeapYear());
    ASSERT_TRUE(!Date(2023, 6, 1).isLeapYear());
    ASSERT_TRUE(Date(2000, 6, 1).isLeapYear());
    ASSERT_TRUE(!Date(1900, 6, 1).isLeapYear());
    ASSERT_TRUE(Date(2024, 2, 1).getDaysInMonth() == 29);
    ASSERT_TRUE(Date(2023, 2, 1).getDaysInMonth() == 28);
    ASSERT_TRUE(Date(2024, 1, 1).getDaysInMonth() == 31);
    ASSERT_TRUE(Date(2024, 4, 1).getDaysInMonth() == 30);
    ASSERT_TRUE(Date(2024, 6, 1).getDaysInYear() == 366);
    ASSERT_TRUE(Date(2023, 6, 1).getDaysInYear() == 365);
}

/* Ordinals count from 1 and include the leap day. */
TEST(Date, OrdinalsCountFromOne) {
    ASSERT_TRUE(Date(2024, 2, 1).getOrdinal() == 32);
    ASSERT_TRUE(Date(2024, 12, 31).getOrdinal() == 366);
    ASSERT_TRUE(Date(2023, 12, 31).getOrdinal() == 365);
}

/* Adding exact durations advances the instant precisely. */
TEST(Date, PlusExactDurations) {
    auto const date = Date(2024, 1, 15, 10, 30, 45, 123);
    auto const sum = date.plus(DateSpan{ .days = 1, .hours = 2, .minutes = 3, .seconds = 4, .milliseconds = 5 });
    ASSERT_TRUE(sum.getDay() == 16);
    ASSERT_TRUE(sum.getHour() == 12);
    ASSERT_TRUE(sum.getMinute() == 33);
    ASSERT_TRUE(sum.getSecond() == 49);
    ASSERT_TRUE(sum.getMillisecond() == 128);
    ASSERT_TRUE(sum.diffMilliseconds(date) == ((26 * 60 + 3) * 60 + 4) * 1000LL + 5);
}

/* Adding months clamps the day to the end of the target month. */
TEST(Date, PlusMonthsClampsDay) {
    auto const jan31 = Date(2023, 1, 31);
    auto const plusMonth = jan31.plus(DateSpan{ .months = 1 });
    ASSERT_TRUE(plusMonth.getYear() == 2023);
    ASSERT_TRUE(plusMonth.getMonth() == 2);
    ASSERT_TRUE(plusMonth.getDay() == 28);

    auto const leap = Date(2024, 1, 31).plus(DateSpan{ .months = 1 });
    ASSERT_TRUE(leap.getMonth() == 2);
    ASSERT_TRUE(leap.getDay() == 29);
}

/* Adding years is calendar-aware across the leap day. */
TEST(Date, PlusYearsCalendarAware) {
    auto const feb29 = Date(2024, 2, 29);
    auto const plusYear = feb29.plus(DateSpan{ .years = 1 });
    ASSERT_TRUE(plusYear.getYear() == 2025);
    ASSERT_TRUE(plusYear.getMonth() == 2);
    ASSERT_TRUE(plusYear.getDay() == 28);
}

/* minus is the inverse of plus for exact durations. */
TEST(Date, MinusIsInverseOfPlus) {
    auto const date = Date(2024, 6, 15, 12, 0, 0, 0);
    auto const span = DateSpan{ .days = 3, .hours = 5, .minutes = 7 };
    ASSERT_TRUE(date.plus(span).minus(span) == date);
    ASSERT_TRUE(date.minus(DateSpan{ .days = 15 }).getMonth() == 5);
    ASSERT_TRUE(date.minus(DateSpan{ .days = 15 }).getDay() == 31);
}

/* startOf truncates to each calendar unit; weeks start Monday. */
TEST(Date, StartOfTruncatesToUnit) {
    auto const date = Date(2024, 8, 17, 13, 45, 30, 500); /* a Saturday */
    ASSERT_TRUE(date.startOf(DateUnit::Year) == Date(2024, 1, 1));
    ASSERT_TRUE(date.startOf(DateUnit::Quarter) == Date(2024, 7, 1));
    ASSERT_TRUE(date.startOf(DateUnit::Month) == Date(2024, 8, 1));
    ASSERT_TRUE(date.startOf(DateUnit::Week) == Date(2024, 8, 12)); /* Monday */
    ASSERT_TRUE(date.startOf(DateUnit::Day) == Date(2024, 8, 17));
    ASSERT_TRUE(date.startOf(DateUnit::Hour) == Date(2024, 8, 17, 13));
    ASSERT_TRUE(date.startOf(DateUnit::Minute) == Date(2024, 8, 17, 13, 45));
    ASSERT_TRUE(date.startOf(DateUnit::Second) == Date(2024, 8, 17, 13, 45, 30));
    ASSERT_TRUE(date.startOf(DateUnit::Millisecond) == date);
}

/* endOf lands on the last millisecond of each unit. */
TEST(Date, EndOfLandsOnLastMillisecond) {
    auto const date = Date(2024, 2, 10, 13, 45, 30, 500);
    ASSERT_TRUE(date.endOf(DateUnit::Year) == Date(2024, 12, 31, 23, 59, 59, 999));
    ASSERT_TRUE(date.endOf(DateUnit::Month) == Date(2024, 2, 29, 23, 59, 59, 999));
    ASSERT_TRUE(date.endOf(DateUnit::Day) == Date(2024, 2, 10, 23, 59, 59, 999));
    ASSERT_TRUE(date.endOf(DateUnit::Hour) == Date(2024, 2, 10, 13, 59, 59, 999));
    ASSERT_TRUE(date.endOf(DateUnit::Millisecond) == date);
}

/* hasSame compares calendar units, not raw instants. */
TEST(Date, HasSameComparesCalendarUnits) {
    auto const morning = Date(2024, 3, 5, 8, 0, 0, 0);
    auto const evening = Date(2024, 3, 5, 22, 30, 0, 0);
    auto const nextDay = Date(2024, 3, 6, 0, 0, 0, 0);
    ASSERT_TRUE(morning.hasSame(evening, DateUnit::Day));
    ASSERT_TRUE(morning.hasSame(evening, DateUnit::Month));
    ASSERT_TRUE(morning.hasSame(evening, DateUnit::Year));
    ASSERT_TRUE(!morning.hasSame(evening, DateUnit::Hour));
    ASSERT_TRUE(!morning.hasSame(nextDay, DateUnit::Day));
    ASSERT_TRUE(morning.hasSame(nextDay, DateUnit::Month));
}

/* diffMilliseconds is signed (this - other). */
TEST(Date, DiffMillisecondsIsSigned) {
    auto const a = Date(2024, 1, 1, 0, 0, 1, 0);
    auto const b = Date(2024, 1, 1, 0, 0, 0, 0);
    ASSERT_TRUE(a.diffMilliseconds(b) == 1000);
    ASSERT_TRUE(b.diffMilliseconds(a) == -1000);
    ASSERT_TRUE(a.diffMilliseconds(a) == 0);
}

/* toISO and toString produce the full UTC ISO-8601 form. */
TEST(Date, ToISOProducesFullForm) {
    auto const date = Date(2024, 1, 15, 10, 30, 45, 123);
    ASSERT_TRUE(date.toISO() == "2024-01-15T10:30:45.123Z");
    ASSERT_TRUE(date.toString() == date.toISO());
    ASSERT_TRUE(Date(0).toISO() == "1970-01-01T00:00:00.000Z");
    ASSERT_TRUE(Date(2024, 9, 5, 1, 2, 3, 4).toISO() == "2024-09-05T01:02:03.004Z");
}

/* FromISO parses date-only, date-time and full-precision forms. */
TEST(Date, FromISOParsesSupportedForms) {
    auto const dateOnly = Date::FromISO("2024-01-15");
    ASSERT_TRUE(dateOnly.has_value());
    ASSERT_TRUE(*dateOnly == Date(2024, 1, 15));

    auto const dateTime = Date::FromISO("2024-01-15T10:30");
    ASSERT_TRUE(dateTime.has_value());
    ASSERT_TRUE(*dateTime == Date(2024, 1, 15, 10, 30));

    auto const full = Date::FromISO("2024-01-15T10:30:45.123Z");
    ASSERT_TRUE(full.has_value());
    ASSERT_TRUE(*full == Date(2024, 1, 15, 10, 30, 45, 123));
}

/* FromISO rejects unparsable text. */
TEST(Date, FromISORejectsUnparsableText) {
    ASSERT_TRUE(!Date::FromISO("").has_value());
    ASSERT_TRUE(!Date::FromISO("not a date").has_value());
}

/* toISO round-trips through FromISO. */
TEST(Date, ToISORoundTripsThroughFromISO) {
    auto const date = Date(2024, 7, 4, 23, 59, 59, 999);
    auto const parsed = Date::FromISO(date.toISO());
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed == date);
}

/* toFormat expands each supported token. */
TEST(Date, ToFormatExpandsTokens) {
    auto const date = Date(2024, 1, 15, 13, 5, 7, 42); /* Monday, 1:05 PM */
    ASSERT_TRUE(date.toFormat("yyyy-MM-dd") == "2024-01-15");
    ASSERT_TRUE(date.toFormat("yy") == "24");
    ASSERT_TRUE(date.toFormat("M/d") == "1/15");
    ASSERT_TRUE(date.toFormat("MMMM") == "January");
    ASSERT_TRUE(date.toFormat("MMM") == "Jan");
    ASSERT_TRUE(date.toFormat("EEEE") == "Monday");
    ASSERT_TRUE(date.toFormat("EEE") == "Mon");
    ASSERT_TRUE(date.toFormat("HH:mm:ss") == "13:05:07");
    ASSERT_TRUE(date.toFormat("H:m:s") == "13:5:7");
    ASSERT_TRUE(date.toFormat("hh:mm a") == "01:05 PM");
    ASSERT_TRUE(date.toFormat("h a") == "1 PM");
    ASSERT_TRUE(date.toFormat("SSS") == "042");
}

/* Twelve-hour formatting maps midnight and noon to 12. */
TEST(Date, TwelveHourMapsMidnightAndNoon) {
    ASSERT_TRUE(Date(2024, 1, 15, 0, 0).toFormat("h a") == "12 AM");
    ASSERT_TRUE(Date(2024, 1, 15, 12, 0).toFormat("h a") == "12 PM");
    ASSERT_TRUE(Date(2024, 1, 15, 11, 59).toFormat("a") == "AM");
}

/* Quoted text is emitted literally and other characters pass through. */
TEST(Date, QuotedTextEmittedLiterally) {
    auto const date = Date(2024, 1, 15, 13, 5);
    ASSERT_TRUE(date.toFormat("'Year' yyyy") == "Year 2024");
    ASSERT_TRUE(date.toFormat("dd 'd' dd") == "15 d 15");
    ASSERT_TRUE(date.toFormat("HH:mm (yyyy)") == "13:05 (2024)");
    ASSERT_TRUE(date.toFormat("") == "");
}

/* Dates before the epoch have negative milliseconds and valid parts. */
TEST(Date, PreEpochDatesAreValid) {
    auto const date = Date(1969, 12, 31, 23, 0, 0, 0);
    ASSERT_TRUE(date.toMilliseconds() < 0);
    ASSERT_TRUE(date.getYear() == 1969);
    ASSERT_TRUE(date.getMonth() == 12);
    ASSERT_TRUE(date.getDay() == 31);
    ASSERT_TRUE(date.getHour() == 23);
    ASSERT_TRUE(date.plus(DateSpan{ .hours = 1 }).toMilliseconds() == 0);
}

/* A default-constructed Date is the current instant and round-trips its millis. */
TEST(Date, DefaultConstructedIsCurrentInstant) {
    auto const now = Date();
    ASSERT_TRUE(now.toMilliseconds() > 1577836800000LL); /* after 2020-01-01 */
    ASSERT_TRUE(now.toMilliseconds() < 4102444800000LL); /* before 2100-01-01 */
    ASSERT_TRUE(Date() >= now);
    ASSERT_TRUE(Date(now.toMilliseconds()) == now);
}
