#include "Date.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>

namespace {

// Days since 1970-01-01 (proleptic Gregorian), from Howard Hinnant's algorithm.
long long daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const long long yoe = y - era * 400;
    const long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

Date civilFromDays(long long z) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const long long doe = z - era * 146097;
    const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long long mp = (5 * doy + 2) / 153;
    const int d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    const int m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    const int y = static_cast<int>(yoe + era * 400 + (m <= 2));
    return Date{y, m, d};
}

}  // namespace

bool Date::operator<(const Date& o) const {
    if (year != o.year) return year < o.year;
    if (month != o.month) return month < o.month;
    return day < o.day;
}

std::string Date::iso() const {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", year, month, day);
    return buf;
}

std::string Date::pretty() const {
    static const std::array<const char*, 12> names = {"January", "February", "March",     "April",   "May",      "June",
                                                      "July",    "August",   "September", "October", "November", "December"};
    std::ostringstream out;
    out << day << ' ' << (month >= 1 && month <= 12 ? names[month - 1] : "?") << ' ' << year;
    return out.str();
}

bool isLeapYear(int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

int daysInMonth(int year, int month) {
    static const std::array<int, 12> days = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    return month == 2 && isLeapYear(year) ? 29 : days[month - 1];
}

bool isValidDate(const Date& d) {
    return d.year >= 1 && d.month >= 1 && d.month <= 12 && d.day >= 1 && d.day <= daysInMonth(d.year, d.month);
}

std::optional<Date> parseDate(const std::string& text) {
    std::string s = text;
    for (char& c : s)
        if (c == '/' || c == '.') c = '-';
    Date d;
    char sep1 = 0, sep2 = 0;
    std::istringstream in(s);
    if (!(in >> d.year >> sep1 >> d.month >> sep2 >> d.day) || sep1 != '-' || sep2 != '-') return std::nullopt;
    std::string rest;
    if (in >> rest) return std::nullopt;
    if (!isValidDate(d)) return std::nullopt;
    return d;
}

Date today() {
    if (const char* forced = std::getenv("FORTUNE_TODAY")) {
        if (auto d = parseDate(forced)) return *d;
    }
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return Date{local.tm_year + 1900, local.tm_mon + 1, local.tm_mday};
}

int ageOn(const Date& birth, const Date& on) {
    int age = on.year - birth.year;
    if (on.month < birth.month || (on.month == birth.month && on.day < birth.day)) --age;
    return age;
}

int weekday(const Date& d) {
    // 1970-01-01 was a Thursday (index 3 when Monday = 0).
    const long long days = daysFromCivil(d.year, d.month, d.day);
    return static_cast<int>(((days % 7) + 7 + 3) % 7);
}

std::string isoWeek(const Date& d) {
    // The ISO week belongs to the year of its Thursday.
    const long long days = daysFromCivil(d.year, d.month, d.day);
    const Date thursday = civilFromDays(days - weekday(d) + 3);
    const long long firstDay = daysFromCivil(thursday.year, 1, 1);
    const int week = static_cast<int>((days - weekday(d) + 3 - firstDay) / 7 + 1);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-W%02d", thursday.year, week);
    return buf;
}

long long daysBetween(const Date& a, const Date& b) {
    return daysFromCivil(b.year, b.month, b.day) - daysFromCivil(a.year, a.month, a.day);
}

int daysUntilBirthday(const Date& birth, const Date& today) {
    for (int year = today.year; year <= today.year + 1; ++year) {
        Date next{year, birth.month, birth.day};
        if (!isValidDate(next)) next.day = 28;  // 29 February in a non-leap year
        const long long d = daysBetween(today, next);
        if (d >= 0) return static_cast<int>(d);
    }
    return 0;
}
