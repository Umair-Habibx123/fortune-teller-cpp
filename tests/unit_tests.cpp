// Small self-contained unit tests (no framework needed). Run: ctest --test-dir build
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "Date.hpp"
#include "Horoscope.hpp"
#include "Json.hpp"
#include "LegacyData.hpp"
#include "Readings.hpp"
#include "Storage.hpp"
#include "Zodiac.hpp"

namespace {

int g_failed = 0;
int g_passed = 0;

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (cond) {                                                                       \
            ++g_passed;                                                                   \
        } else {                                                                          \
            ++g_failed;                                                                   \
            std::cerr << __FILE__ << ":" << __LINE__ << ": FAILED: " << #cond << '\n';    \
        }                                                                                 \
    } while (0)

void dates() {
    CHECK(isLeapYear(2024) && !isLeapYear(2023) && !isLeapYear(1900) && isLeapYear(2000));
    CHECK(parseDate("2024-02-29").has_value());
    CHECK(!parseDate("2023-02-29").has_value());
    CHECK(!parseDate("2023-04-31").has_value());   // the original's check missed this
    CHECK(!parseDate("2023-13-01").has_value());
    CHECK(!parseDate("hello").has_value());
    CHECK(parseDate("2002/05/14") == (Date{2002, 5, 14}));
    CHECK(ageOn({2002, 5, 14}, {2026, 5, 13}) == 23);
    CHECK(ageOn({2002, 5, 14}, {2026, 5, 14}) == 24);
    CHECK(isoWeek({2026, 11, 15}) == "2026-W46");   // a Sunday
    CHECK(isoWeek({2026, 11, 9}) == "2026-W46");    // the Monday of that week
    CHECK(isoWeek({2021, 1, 3}) == "2020-W53");     // belongs to the previous ISO year
    CHECK(weekday({2026, 11, 9}) == 0);             // Monday
}

void zodiac() {
    CHECK(signFor(3, 21) == Sign::Aries);
    CHECK(signFor(3, 20) == Sign::Pisces);
    CHECK(signFor(1, 19) == Sign::Capricorn);
    CHECK(signFor(1, 20) == Sign::Aquarius);
    CHECK(signFor(12, 22) == Sign::Capricorn);
    CHECK(signFor(12, 21) == Sign::Sagittarius);
    CHECK(signFor(11, 22) == Sign::Sagittarius);
    CHECK(signFor(5, 14) == Sign::Taurus);
    CHECK(parseSign("SAGGITARIUS") == Sign::Sagittarius);  // the original spelling
    CHECK(parseSign(" leo ") == Sign::Leo);
    CHECK(parseSign("12") == Sign::Pisces);
    CHECK(!parseSign("ophiuchus").has_value());

    // every unordered pair maps to its own text, in both orders
    std::set<int> seen;
    for (int a = 0; a < kSignCount; ++a)
        for (int b = 0; b < kSignCount; ++b) {
            CHECK(compatibilityIndex(signAt(a), signAt(b)) == compatibilityIndex(signAt(b), signAt(a)));
            seen.insert(compatibilityIndex(signAt(a), signAt(b)));
        }
    CHECK(seen.size() == 78 && *seen.begin() == 0 && *seen.rbegin() == 77);
    CHECK(legacy::kLove.size() == 78);
    // spot-check the pairs the original program mixed up
    CHECK(compatibility(Sign::Scorpio, Sign::Sagittarius).find("Sagittarius") != std::string::npos);
    CHECK(compatibility(Sign::Aquarius, Sign::Gemini).find("Aquarius") != std::string::npos);
}

void readings() {
    CHECK(legacy::kAbout.size() == 12 && legacy::kArt.size() == 12);
    for (const auto& notes : legacy::kAbout) CHECK(notes.size() >= 25);
    CHECK(aboutYou(Sign::Aries, 14).size() == 5);   // step 4: notes 4, 8, 12, 16, 20
    CHECK(aboutYou(Sign::Aries, 14)[0] == legacy::kAbout[0][3]);
    CHECK(aboutYou(Sign::Aries, 9).size() == 3);    // step 9: 9, 18, 27 (36 is past the 28 notes)
    CHECK(luckyNumbers(14) == (std::vector<int>{14, 21, 15, 18}));
    CHECK(!fortuneCookie().empty());
    for (const auto& f : legacy::kFortunes) CHECK(f.find("STUPID") == std::string::npos);
}

void json() {
    const std::string body = R"({"data":{"date":"2026-11-09","period":"weekly","sign":"Aries",)"
                             R"("horoscope":"Say \"hi\" – café 😀\nnext"}})";
    CHECK(jsonString(body, "date") == std::string("2026-11-09"));
    CHECK(jsonString(body, "horoscope") == std::string("Say \"hi\" \xE2\x80\x93 caf\xC3\xA9 \xF0\x9F\x98\x80\nnext"));
    CHECK(!jsonString(body, "missing").has_value());
    CHECK(jsonString(R"({"error":"bad sign","code":"invalid_sign"})", "code") == std::string("invalid_sign"));
    CHECK(!jsonString(R"({"date": 5})", "date").has_value());
}

void horoscopeCache() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "fortune-unit-cache";
    fs::remove_all(dir);
    fs::create_directories(dir);
    {
        std::ofstream out(dir / "weekly-leo.txt");
        out << "2026-11-10\n2026-11-09\nA saved weekly reading for Leo.\n";
    }
#ifdef _WIN32
    _putenv_s("FORTUNE_OFFLINE", "1");
#else
    setenv("FORTUNE_OFFLINE", "1", 1);
#endif
    HoroscopeService service(dir);
    // same ISO week as the saved one: served from the cache
    auto same = service.get(Sign::Leo, Period::Weekly, {2026, 11, 15});
    CHECK(same.reading && same.reading->fromCache && !same.reading->stale && same.problem.empty());
    // a new week, offline: the old one is returned, marked stale, with the reason
    auto later = service.get(Sign::Leo, Period::Weekly, {2026, 11, 17});
    CHECK(later.reading && later.reading->stale && !later.problem.empty());
    // nothing saved and offline: no reading, just the reason
    auto none = service.get(Sign::Pisces, Period::Daily, {2026, 11, 17});
    CHECK(!none.reading && !none.problem.empty());
    CHECK(HoroscopeService::url(Sign::Sagittarius, Period::Weekly) ==
          "https://freehoroscopeapi.com/api/v1/get-horoscope/weekly?sign=Sagittarius");
    fs::remove_all(dir);
}

void people() {
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path() / "fortune-unit-people.tsv";
    fs::remove(file);
    {
        PeopleBook book(file);
        book.add({"Ayesha Khan", {2001, 3, 25}, {2026, 1, 1}});
        book.add({"Bilal Ahmed", {1999, 3, 2}, {2026, 1, 2}});
        book.add({"Ayesha Khan", {2001, 3, 25}, {2026, 1, 3}});  // same person again: no duplicate
        CHECK(book.all().size() == 2);
        CHECK(book.lastUsed()->name == "Ayesha Khan");
    }
    PeopleBook reloaded(file);
    CHECK(reloaded.all().size() == 2);
    CHECK(reloaded.bornOn(3, 0).size() == 2);
    CHECK(reloaded.bornOn(3, 2).size() == 1);
    CHECK(reloaded.remove(0) && reloaded.all().size() == 1);
    CHECK(!reloaded.remove(5));
    fs::remove(file);
}

}  // namespace

int main() {
    dates();
    zodiac();
    readings();
    json();
    horoscopeCache();
    people();
    std::cout << g_passed << " checks passed, " << g_failed << " failed\n";
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
