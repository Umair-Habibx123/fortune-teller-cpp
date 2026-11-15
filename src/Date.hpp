#pragma once
// Calendar dates: validation (with leap years), age, and ISO weeks.
#include <optional>
#include <string>

struct Date {
    int year = 0;
    int month = 0;  // 1..12
    int day = 0;    // 1..31

    bool operator==(const Date& o) const { return year == o.year && month == o.month && day == o.day; }
    bool operator!=(const Date& o) const { return !(*this == o); }
    bool operator<(const Date& o) const;

    // "2026-11-15"
    std::string iso() const;
    // "15 November 2026"
    std::string pretty() const;
};

bool isLeapYear(int year);
int daysInMonth(int year, int month);
bool isValidDate(const Date& d);

// Parses "YYYY-MM-DD" (also accepts "/" or "." as separators).
std::optional<Date> parseDate(const std::string& text);

// Today's local date. FORTUNE_TODAY=YYYY-MM-DD overrides it (used by the tests).
Date today();

// Whole years between `birth` and `on`.
int ageOn(const Date& birth, const Date& on);

// ISO-8601 week label, e.g. "2026-W46". Weeks start on Monday.
std::string isoWeek(const Date& d);

// 0 = Monday ... 6 = Sunday
int weekday(const Date& d);
