#pragma once
// The 12 Western zodiac signs: which one a birthday falls in, and basic facts.
#include <optional>
#include <string>

#include "Date.hpp"

enum class Sign { Aries, Taurus, Gemini, Cancer, Leo, Virgo, Libra, Scorpio, Sagittarius, Capricorn, Aquarius, Pisces };

constexpr int kSignCount = 12;

struct SignInfo {
    const char* name;      // "Aries"
    const char* title;     // "The Ram"
    const char* dates;     // "Mar 21 - Apr 19"
    const char* element;   // Fire, Earth, Air, Water
    const char* planet;    // ruling planet(s)
};

const SignInfo& info(Sign s);
int index(Sign s);
Sign signAt(int index);  // 0..11

// Same date ranges as the original program.
Sign signFor(int month, int day);
inline Sign signFor(const Date& d) { return signFor(d.month, d.day); }

// "aries", "Sagittarius", "SAGGITARIUS" (the original spelling) ...
std::optional<Sign> parseSign(const std::string& text);

// Position of a pair in legacy::kLove (order of the two signs doesn't matter).
int compatibilityIndex(Sign a, Sign b);
