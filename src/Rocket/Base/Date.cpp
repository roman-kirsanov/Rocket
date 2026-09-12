/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstdio>
#include <cstring>
#include <chrono>
#include <format>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/Time.hpp>
#include <Rocket/Base/Date.hpp>

namespace Rocket {

static char const* _monthsFull[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static char const* _monthsAbbr[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char const* _weekdaysFull[7] = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

static char const* _weekdaysAbbr[7] = {
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

static std::chrono::sys_time<std::chrono::milliseconds> _toTimePoint(std::int64_t millis) {
    PROFILE

    return std::chrono::sys_time<std::chrono::milliseconds>{ std::chrono::milliseconds{ millis } };
}

bool Date::operator==(Date const& other) const {
    PROFILE

    return _millis == other._millis;
}

bool Date::operator!=(Date const& other) const {
    PROFILE

    return _millis != other._millis;
}

bool Date::operator<(Date const& other) const {
    PROFILE

    return _millis < other._millis;
}

bool Date::operator>(Date const& other) const {
    PROFILE

    return _millis > other._millis;
}

bool Date::operator<=(Date const& other) const {
    PROFILE

    return _millis <= other._millis;
}

bool Date::operator>=(Date const& other) const {
    PROFILE

    return _millis >= other._millis;
}

int Date::getYear() const {
    PROFILE

    return int(std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(_toTimePoint(_millis)) }.year());
}

int Date::getMonth() const {
    PROFILE

    return (int)unsigned(std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(_toTimePoint(_millis)) }.month());
}

int Date::getDay() const {
    PROFILE

    return (int)unsigned(std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(_toTimePoint(_millis)) }.day());
}

int Date::getHour() const {
    PROFILE

    auto const tp = _toTimePoint(_millis);

    return (int)std::chrono::hh_mm_ss{ tp - std::chrono::floor<std::chrono::days>(tp) }.hours().count();
}

int Date::getMinute() const {
    PROFILE

    auto const tp = _toTimePoint(_millis);

    return (int)std::chrono::hh_mm_ss{ tp - std::chrono::floor<std::chrono::days>(tp) }.minutes().count();
}

int Date::getSecond() const {
    PROFILE

    auto const tp = _toTimePoint(_millis);

    return (int)std::chrono::hh_mm_ss{ tp - std::chrono::floor<std::chrono::days>(tp) }.seconds().count();
}

int Date::getMillisecond() const {
    PROFILE

    auto const tp = _toTimePoint(_millis);

    return (int)std::chrono::hh_mm_ss{ tp - std::chrono::floor<std::chrono::days>(tp) }.subseconds().count();
}

int Date::getWeekday() const {
    PROFILE

    return (int)std::chrono::weekday{ std::chrono::floor<std::chrono::days>(_toTimePoint(_millis)) }.iso_encoding();
}

int Date::getOrdinal() const {
    PROFILE

    auto const dp = std::chrono::floor<std::chrono::days>(_toTimePoint(_millis));
    auto const ymd = std::chrono::year_month_day{ dp };
    auto const jan1 = std::chrono::sys_days{ ymd.year() / std::chrono::January / 1 };

    return (int)(dp - jan1).count() + 1;
}

int Date::getQuarter() const {
    PROFILE

    return (getMonth() - 1) / 3 + 1;
}

int Date::getDaysInMonth() const {
    PROFILE

    auto const ymd = std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(_toTimePoint(_millis)) };

    return (int)unsigned((ymd.year() / ymd.month() / std::chrono::last).day());
}

int Date::getDaysInYear() const {
    PROFILE

    return isLeapYear() ? 366 : 365;
}

bool Date::isLeapYear() const {
    PROFILE

    return std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(_toTimePoint(_millis)) }.year().is_leap();
}

Date Date::plus(DateSpan const& span) const {
    PROFILE

    auto const tp = _toTimePoint(_millis);
    auto const dp = std::chrono::floor<std::chrono::days>(tp);
    auto const tod = tp - dp;
    auto const ymd = std::chrono::year_month_day{ dp };

    auto ym = std::chrono::year_month{ ymd.year(), ymd.month() };
    ym += std::chrono::months{ span.years * 12 + span.months };

    auto target = std::chrono::year_month_day{ ym / ymd.day() };
    if (target.ok() == false) {
        target = ym / std::chrono::last;
    }

    auto const result = std::chrono::sys_time<std::chrono::milliseconds>{ std::chrono::sys_days{ target } } + tod
        + std::chrono::days{ span.days } + std::chrono::hours{ span.hours } + std::chrono::minutes{ span.minutes }
        + std::chrono::seconds{ span.seconds } + std::chrono::milliseconds{ span.milliseconds };

    return Date{ result.time_since_epoch().count() };
}

Date Date::minus(DateSpan const& span) const {
    PROFILE

    return plus(DateSpan{
        -span.years, -span.months, -span.days,
        -span.hours, -span.minutes, -span.seconds, -span.milliseconds
    });
}

Date Date::startOf(DateUnit unit) const {
    PROFILE

    auto const tp = _toTimePoint(_millis);
    auto const dp = std::chrono::floor<std::chrono::days>(tp);
    auto const ymd = std::chrono::year_month_day{ dp };

    switch (unit) {
        case DateUnit::Year:
            return Date(int(ymd.year()), 1, 1);
        case DateUnit::Quarter:
            return Date(int(ymd.year()), (getQuarter() - 1) * 3 + 1, 1);
        case DateUnit::Month:
            return Date(int(ymd.year()), (int)unsigned(ymd.month()), 1);
        case DateUnit::Week:
            return Date{ std::chrono::sys_time<std::chrono::milliseconds>{ dp - std::chrono::days{ getWeekday() - 1 } }.time_since_epoch().count() };
        case DateUnit::Day:
            return Date{ std::chrono::sys_time<std::chrono::milliseconds>{ dp }.time_since_epoch().count() };
        case DateUnit::Hour:
            return Date{ std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::floor<std::chrono::hours>(tp).time_since_epoch()).count() };
        case DateUnit::Minute:
            return Date{ std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::floor<std::chrono::minutes>(tp).time_since_epoch()).count() };
        case DateUnit::Second:
            return Date{ std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::floor<std::chrono::seconds>(tp).time_since_epoch()).count() };
        case DateUnit::Millisecond:
            return *this;
    }

    return *this;
}

Date Date::endOf(DateUnit unit) const {
    PROFILE

    auto one = DateSpan{};

    switch (unit) {
        case DateUnit::Year:        one.years = 1;   break;
        case DateUnit::Quarter:     one.months = 3;  break;
        case DateUnit::Month:       one.months = 1;  break;
        case DateUnit::Week:        one.days = 7;    break;
        case DateUnit::Day:         one.days = 1;    break;
        case DateUnit::Hour:        one.hours = 1;   break;
        case DateUnit::Minute:      one.minutes = 1; break;
        case DateUnit::Second:      one.seconds = 1; break;
        case DateUnit::Millisecond: return *this;
    }

    return Date{ startOf(unit).plus(one)._millis - 1 };
}

bool Date::hasSame(Date const& other, DateUnit unit) const {
    PROFILE

    return startOf(unit)._millis == other.startOf(unit)._millis;
}

std::int64_t Date::diffMilliseconds(Date const& other) const {
    PROFILE

    return _millis - other._millis;
}

std::int64_t Date::toMilliseconds() const {
    PROFILE

    return _millis;
}

std::string Date::toISO() const {
    PROFILE

    return std::format(
        "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}Z",
        getYear(), getMonth(), getDay(), getHour(), getMinute(), getSecond(), getMillisecond()
    );
}

std::string Date::toFormat(std::string const& pattern) const {
    PROFILE

    auto out = std::string();
    auto i = std::size_t(0);

    auto starts = [&](char const* token) {
        return pattern.compare(i, std::strlen(token), token) == 0;
    };

    while (i < pattern.size()) {
        if (pattern[i] == '\'') {
            i += 1;
            while ((i < pattern.size()) && (pattern[i] != '\'')) {
                out += pattern[i];
                i += 1;
            }
            if (i < pattern.size()) {
                i += 1;
            }
        }
        else if (starts("yyyy")) { out += std::format("{:04d}", getYear());                       i += 4; }
        else if (starts("yy"))   { out += std::format("{:02d}", ((getYear() % 100) + 100) % 100);  i += 2; }
        else if (starts("MMMM")) { out += _monthsFull[getMonth() - 1];                             i += 4; }
        else if (starts("MMM"))  { out += _monthsAbbr[getMonth() - 1];                             i += 3; }
        else if (starts("MM"))   { out += std::format("{:02d}", getMonth());                       i += 2; }
        else if (starts("M"))    { out += std::format("{}", getMonth());                           i += 1; }
        else if (starts("dd"))   { out += std::format("{:02d}", getDay());                         i += 2; }
        else if (starts("d"))    { out += std::format("{}", getDay());                             i += 1; }
        else if (starts("EEEE")) { out += _weekdaysFull[getWeekday() - 1];                         i += 4; }
        else if (starts("EEE"))  { out += _weekdaysAbbr[getWeekday() - 1];                         i += 3; }
        else if (starts("HH"))   { out += std::format("{:02d}", getHour());                        i += 2; }
        else if (starts("H"))    { out += std::format("{}", getHour());                            i += 1; }
        else if (starts("hh"))   { auto h = getHour() % 12; if (h == 0) { h = 12; } out += std::format("{:02d}", h); i += 2; }
        else if (starts("h"))    { auto h = getHour() % 12; if (h == 0) { h = 12; } out += std::format("{}", h);     i += 1; }
        else if (starts("mm"))   { out += std::format("{:02d}", getMinute());                      i += 2; }
        else if (starts("m"))    { out += std::format("{}", getMinute());                          i += 1; }
        else if (starts("ss"))   { out += std::format("{:02d}", getSecond());                      i += 2; }
        else if (starts("s"))    { out += std::format("{}", getSecond());                          i += 1; }
        else if (starts("SSS"))  { out += std::format("{:03d}", getMillisecond());                 i += 3; }
        else if (starts("a"))    { out += (getHour() < 12) ? "AM" : "PM";                          i += 1; }
        else { out += pattern[i]; i += 1; }
    }

    return out;
}

std::string Date::toString() const {
    PROFILE

    return toISO();
}

std::optional<Date> Date::FromISO(std::string const& text) {
    PROFILE

    int y = 0, mo = 1, d = 1, h = 0, mi = 0, s = 0, ms = 0;

    if (std::sscanf(text.c_str(), "%d-%d-%dT%d:%d:%d.%d", &y, &mo, &d, &h, &mi, &s, &ms) >= 3) {
        return Date{ y, mo, d, h, mi, s, ms };
    }

    return std::nullopt;
}

Date::Date()
    : _millis(GetTime()) {
    PROFILE
}

Date::Date(std::int64_t millis)
    : _millis(millis) {
    PROFILE
}

Date::Date(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    PROFILE

    auto const ymd = std::chrono::year{ year } / std::chrono::month{ (unsigned)month } / std::chrono::day{ (unsigned)day };
    auto const tp = std::chrono::sys_time<std::chrono::milliseconds>{ std::chrono::sys_days{ ymd } }
        + std::chrono::hours{ hour } + std::chrono::minutes{ minute } + std::chrono::seconds{ second } + std::chrono::milliseconds{ millisecond };

    _millis = tp.time_since_epoch().count();
}

} /* namespace Rocket */
