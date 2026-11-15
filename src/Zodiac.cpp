#include "Zodiac.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace {

const std::array<SignInfo, kSignCount> kSigns = {{
    {"Aries", "The Ram", "Mar 21 - Apr 19", "Fire", "Mars"},
    {"Taurus", "The Bull", "Apr 20 - May 20", "Earth", "Venus"},
    {"Gemini", "The Twins", "May 21 - Jun 20", "Air", "Mercury"},
    {"Cancer", "The Crab", "Jun 21 - Jul 22", "Water", "Moon"},
    {"Leo", "The Lion", "Jul 23 - Aug 22", "Fire", "Sun"},
    {"Virgo", "The Maiden", "Aug 23 - Sep 22", "Earth", "Mercury"},
    {"Libra", "The Scales", "Sep 23 - Oct 22", "Air", "Venus"},
    {"Scorpio", "The Scorpion", "Oct 23 - Nov 21", "Water", "Pluto and Mars"},
    {"Sagittarius", "The Archer", "Nov 22 - Dec 21", "Fire", "Jupiter"},
    {"Capricorn", "The Goat", "Dec 22 - Jan 19", "Earth", "Saturn"},
    {"Aquarius", "The Water Bearer", "Jan 20 - Feb 18", "Air", "Uranus and Saturn"},
    {"Pisces", "The Fishes", "Feb 19 - Mar 20", "Water", "Neptune and Jupiter"},
}};

// First day of each sign, by month: {month, day, sign}.
struct Start {
    int month, day;
    Sign sign;
};
const std::array<Start, kSignCount> kStarts = {{
    {1, 20, Sign::Aquarius},  {2, 19, Sign::Pisces},  {3, 21, Sign::Aries},   {4, 20, Sign::Taurus},
    {5, 21, Sign::Gemini},    {6, 21, Sign::Cancer},  {7, 23, Sign::Leo},     {8, 23, Sign::Virgo},
    {9, 23, Sign::Libra},     {10, 23, Sign::Scorpio}, {11, 22, Sign::Sagittarius}, {12, 22, Sign::Capricorn},
}};

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

const SignInfo& info(Sign s) { return kSigns[static_cast<std::size_t>(s)]; }
int index(Sign s) { return static_cast<int>(s); }
Sign signAt(int i) { return static_cast<Sign>(((i % kSignCount) + kSignCount) % kSignCount); }

Sign signFor(int month, int day) {
    // Before Jan 20 it is still Capricorn from the previous December.
    Sign current = Sign::Capricorn;
    for (const Start& s : kStarts) {
        if (month > s.month || (month == s.month && day >= s.day)) current = s.sign;
    }
    return current;
}

std::optional<Sign> parseSign(const std::string& text) {
    std::string t = lower(text);
    t.erase(std::remove_if(t.begin(), t.end(), [](unsigned char c) { return std::isspace(c); }), t.end());
    if (t == "saggitarius") t = "sagittarius";  // spelling used by the original program
    for (int i = 0; i < kSignCount; ++i) {
        if (lower(kSigns[static_cast<std::size_t>(i)].name) == t) return signAt(i);
    }
    // a number 1..12 also works
    if (!t.empty() && std::all_of(t.begin(), t.end(), [](unsigned char c) { return std::isdigit(c); }) && t.size() <= 2) {
        const int n = std::stoi(t);
        if (n >= 1 && n <= kSignCount) return signAt(n - 1);
    }
    return std::nullopt;
}

int compatibilityIndex(Sign a, Sign b) {
    int i = index(a), j = index(b);
    if (i > j) std::swap(i, j);
    // rows Aries..Pisces, each row starting at its own sign: 12 + 11 + 10 + ...
    return i * kSignCount - i * (i - 1) / 2 + (j - i);
}
