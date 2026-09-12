/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace Rocket {

/** A calendar unit, used by startOf / endOf / hasSame. */
enum class DateUnit {
    Year,
    Quarter,
    Month,
    Week,
    Day,
    Hour,
    Minute,
    Second,
    Millisecond
};

/**
 * A calendar span used as input to Date::plus / Date::minus. Fields combine
 * additively; years and months are applied calendar-aware (clamping the day to
 * the end of the target month), the rest as exact durations.
 */
struct DateSpan {
    int years = 0;
    int months = 0;
    int days = 0;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int milliseconds = 0;
};

/**
 * An immutable instant on the timeline with calendar accessors and arithmetic,
 * backed by milliseconds since the Unix epoch. All components are read in UTC;
 * there is no time-zone or locale support (formatting uses fixed English
 * names). Round-trips with GetTime() and the App timers via toMilliseconds().
 */
class Date {
public:
    /** Constructs the current instant (from GetTime()). */
    Date();

    /**
     * An instant from UTC calendar components (month 1-12, day 1-31).
     *
     * @param year        Full year.
     * @param month       Month, 1 (January) to 12 (December).
     * @param day         Day of the month, 1 to 31.
     * @param hour        Hour, 0 to 23, default 0.
     * @param minute      Minute, 0 to 59, default 0.
     * @param second      Second, 0 to 59, default 0.
     * @param millisecond Millisecond, 0 to 999, default 0.
     */
    Date(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int millisecond = 0);

    /**
     * An instant from milliseconds since the Unix epoch.
     *
     * @param millis Milliseconds since the Unix epoch.
     */
    explicit Date(std::int64_t millis);

    bool operator==(Date const&) const;
    bool operator!=(Date const&) const;
    bool operator<(Date const&) const;
    bool operator>(Date const&) const;
    bool operator<=(Date const&) const;
    bool operator>=(Date const&) const;

    /** Full year. */
    int getYear() const;

    /** Month, 1 (January) to 12 (December). */
    int getMonth() const;

    /** Day of the month, 1 to 31. */
    int getDay() const;

    /** Hour, 0 to 23. */
    int getHour() const;

    /** Minute, 0 to 59. */
    int getMinute() const;

    /** Second, 0 to 59. */
    int getSecond() const;

    /** Millisecond, 0 to 999. */
    int getMillisecond() const;

    /** ISO weekday, 1 (Monday) to 7 (Sunday). */
    int getWeekday() const;

    /** Day of the year, 1 to 366. */
    int getOrdinal() const;

    /** Quarter, 1 to 4. */
    int getQuarter() const;

    /** Number of days in this date's month. */
    int getDaysInMonth() const;

    /** Number of days in this date's year (365 or 366). */
    int getDaysInYear() const;

    /** Whether this date's year is a leap year. */
    bool isLeapYear() const;

    /**
     * Returns a new Date advanced by `span`.
     *
     * @param span The calendar span to add.
     */
    Date plus(DateSpan const& span) const;

    /**
     * Returns a new Date moved back by `span`.
     *
     * @param span The calendar span to subtract.
     */
    Date minus(DateSpan const& span) const;

    /**
     * Returns a new Date at the start of the given unit (weeks start Monday).
     *
     * @param unit The calendar unit to align to.
     */
    Date startOf(DateUnit unit) const;

    /**
     * Returns a new Date at the last millisecond of the given unit.
     *
     * @param unit The calendar unit to align to.
     */
    Date endOf(DateUnit unit) const;

    /**
     * Whether two dates fall in the same calendar unit (e.g. same day).
     *
     * @param other The date to compare against.
     * @param unit  The calendar unit to compare within.
     */
    bool hasSame(Date const& other, DateUnit unit) const;

    /**
     * Signed difference in milliseconds (this - other).
     *
     * @param other The date to compare against.
     */
    std::int64_t diffMilliseconds(Date const& other) const;

    /** Milliseconds since the Unix epoch. */
    std::int64_t toMilliseconds() const;

    /** ISO-8601 string in UTC: YYYY-MM-DDTHH:MM:SS.sssZ. */
    std::string toISO() const;

    /**
     * Formats using token replacement: yyyy yy, MMMM MMM MM M, dd d,
     * EEEE EEE, HH H, hh h, mm m, ss s, SSS, a. Text inside single quotes is
     * emitted literally.
     *
     * @param pattern The format string containing tokens and literal text.
     */
    std::string toFormat(std::string const& pattern) const;

    /** Same as toISO(). */
    std::string toString() const;

    /**
     * Parses an ISO-8601 string (YYYY-MM-DD[THH:MM[:SS[.sss]]][Z]); std::nullopt on failure.
     *
     * @param text The ISO-8601 string to parse.
     */
    static std::optional<Date> FromISO(std::string const& text);
private:
    std::int64_t _millis;
};

} /* namespace Rocket */
