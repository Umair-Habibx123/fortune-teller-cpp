// Small self-contained unit tests (no framework needed). Run: ctest --test-dir build
#include <cmath>
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
#include "Sky.hpp"
#include "Storage.hpp"
#include "Tarot.hpp"
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

void setOffline() {
#ifdef _WIN32
    _putenv_s("FORTUNE_OFFLINE", "1");
#else
    setenv("FORTUNE_OFFLINE", "1", 1);
#endif
}

void jsonParser() {
    auto doc = Json::parse(R"( {"a":[1, -2.5e1, true, null, "xé"], "b":{"c":{}}, "d":false} )");
    CHECK(doc.has_value());
    if (!doc) return;
    const Json& j = *doc;
    CHECK(j.isObject() && j.size() == 3);
    CHECK(j["a"].isArray() && j["a"].size() == 5);
    CHECK(j["a"][0].num() == 1.0);
    CHECK(j["a"][1].num() == -25.0);
    CHECK(j["a"][2].boolean());
    CHECK(j["a"][3].isNull());
    CHECK(j["a"][4].str() == "x\xC3\xA9");
    CHECK(j["b"]["c"].isObject() && j["b"]["c"].size() == 0);
    CHECK(!j["d"].boolean(true));
    CHECK(j["missing"]["deeper"][3].isNull());    // missing paths are safe
    CHECK(j["a"].str("fallback") == "fallback");  // wrong type -> fallback
    CHECK(j.has("b") && !j.has("z"));
    CHECK(!Json::parse("{\"a\":1,}").has_value());
    CHECK(!Json::parse("[1 2]").has_value());
    CHECK(!Json::parse("{} trailing").has_value());
    CHECK(!Json::parse("\"unterminated").has_value());
    auto emoji = Json::parse("\"\\ud83d\\ude00\"");  // surrogate pair
    CHECK(emoji && emoji->str() == "\xF0\x9F\x98\x80");
}

const char* kDeck = R"({"nhits":3,"cards":[
 {"type":"major","name":"The Fool","name_short":"ar00","value":"0","value_int":0,"meaning_up":"Beginnings.","meaning_rev":"Recklessness.","desc":"A young man."},
 {"type":"minor","name":"Three of Cups","name_short":"cu03","value":"three","value_int":3,"suit":"cups","meaning_up":"Friendship.","meaning_rev":"Excess.","desc":"Three maidens."},
 {"type":"minor","name":"Queen of Swords","name_short":"swqu","value":"queen","value_int":13,"suit":"swords","meaning_up":"Clarity.","meaning_rev":"Coldness.","desc":"A queen."}]})";

void tarot() {
    namespace fs = std::filesystem;
    const auto deck = Tarot::parseDeck(*Json::parse(kDeck));
    CHECK(deck.size() == 3);
    CHECK(deck.size() == 3 && deck[1].name == "Three of Cups" && deck[1].suit == "cups" && deck[1].arcana == "minor" && deck[1].value == 3);
    CHECK(!deck.empty() && deck[0].meaningReversed == "Recklessness.");

    const fs::path dir = fs::temp_directory_path() / "fortune-unit-tarot";
    fs::remove_all(dir);
    fs::create_directories(dir);
    { std::ofstream(dir / "tarot-deck.json") << "2026-01-01\n" << kDeck; }
    setOffline();
    Tarot t(dir);
    CHECK(t.load({2026, 11, 15}).empty() && t.ready());  // the saved deck never expires
    const DrawnCard a = t.cardOfTheDay("Ayesha", {2026, 11, 15});
    const DrawnCard b = t.cardOfTheDay("Ayesha", {2026, 11, 15});
    CHECK(a.card.name == b.card.name && a.reversed == b.reversed);  // stable all day
    std::set<std::string> seen;
    for (int day = 1; day <= 28; ++day) seen.insert(t.cardOfTheDay("Ayesha", {2026, 2, day}).card.name);
    CHECK(seen.size() > 1);  // but changes between days
    const auto three = t.draw(3);
    std::set<std::string> names;
    for (const auto& c : three) names.insert(c.card.name);
    CHECK(three.size() == 3 && names.size() == 3);  // no repeats in a spread
    CHECK(t.draw(10).size() == 3);                  // can't draw more than the deck
    CHECK(t.search("queen").size() == 1 && t.search("QUEEN OF").size() == 1);
    CHECK(t.group("major").size() == 1 && t.group("cups").size() == 1 && t.group("wands").empty());

    Tarot empty(dir / "nothing-here");
    CHECK(!empty.load({2026, 11, 15}).empty() && !empty.ready());  // offline, nothing saved
    fs::remove_all(dir);
}

void sky() {
    namespace fs = std::filesystem;
    CHECK(clockTime("2026-11-15T06:35:10+05:00", true) == "06:35");
    CHECK(clockTime("2026-11-15T06:35:10+05:00", false) == "6:35 AM");
    CHECK(clockTime("2026-11-15T17:02:00", false) == "5:02 PM");
    CHECK(clockTime("2026-11-15T00:15:00", false) == "12:15 AM");
    CHECK(clockTime("2026-11-15T12:00:00", false) == "12:00 PM");
    CHECK(clockTime("", true) == "-");
    CHECK(duration(38520) == "10h 42m");
    CHECK(planetName("sun") == "Sun" && planetName("rahu") == "Rahu (north node)");

    const auto doc = Json::parse(R"({"datetime":"2026-11-15T12:00:00Z","zodiac":"sidereal","bodies":{
        "moon":{"sign":"Leo","degrees_in_sign":3.5,"retrograde":false,"nakshatra":"Magha","pada":2},
        "sun":{"sign":"Libra","degrees_in_sign":29.1,"retrograde":false},
        "saturn":{"sign":"Pisces","degrees_in_sign":1.0,"retrograde":true}}})");
    CHECK(doc.has_value());
    if (!doc) return;
    const Planets p = parsePlanets(*doc);
    CHECK(p.calculatedAt == "2026-11-15T12:00:00Z");
    CHECK(p.bodies.size() == 3);
    if (p.bodies.size() == 3) {
        CHECK(p.bodies[0].body == "sun" && p.bodies[1].body == "moon");  // traditional order, not JSON order
        CHECK(p.bodies[1].nakshatra == "Magha" && p.bodies[1].pada == 2);
        CHECK(p.bodies[2].retrograde && std::fabs(p.bodies[2].degreesInSign - 1.0) < 1e-9);
    }

    CHECK(findCity("london") && findCity("london")->name == "London, UK");
    CHECK(findCity("new y") && findCity("new y")->name == "New York, USA");
    CHECK(!findCity("atlantis") && !findCity(""));

    const fs::path file = fs::temp_directory_path() / "fortune-unit-settings.txt";
    fs::remove(file);
    const Settings defaults = loadSettings(file);
    CHECK(!defaults.location && !defaults.sidereal && defaults.clock24);
    Settings s;
    s.location = Location{"Custom location", -33.8688, 151.2093};
    s.sidereal = true;
    s.clock24 = false;
    saveSettings(file, s);
    const Settings back = loadSettings(file);
    CHECK(back.location && back.location->name == "Custom location");
    CHECK(back.location && std::fabs(back.location->lat + 33.8688) < 1e-6 && std::fabs(back.location->lng - 151.2093) < 1e-6);
    CHECK(back.sidereal && !back.clock24);
    { std::ofstream(file) << "lat=200\nlng=10\n"; }
    CHECK(!loadSettings(file).location);  // out-of-range coordinates are ignored
    fs::remove(file);

    const Sky noPlace(fs::temp_directory_path(), Settings{});
    CHECK(!noPlace.sunMoon({2026, 11, 15}, {2026, 11, 15}).data);  // needs a city first
}

void extras() {
    CHECK(daysUntilBirthday({2002, 5, 14}, {2026, 5, 14}) == 0);
    CHECK(daysUntilBirthday({2002, 5, 14}, {2026, 5, 13}) == 1);
    CHECK(daysUntilBirthday({2002, 5, 14}, {2026, 5, 15}) == 364);
    CHECK(daysUntilBirthday({2000, 2, 29}, {2026, 2, 27}) == 1);  // leap-day birthdays move to 28 Feb
    CHECK(daysUntilBirthday({2002, 1, 1}, {2026, 12, 31}) == 1);
    CHECK(chineseAnimal(2020) == "Rat" && chineseAnimal(2002) == "Horse" && chineseAnimal(1999) == "Rabbit");
    CHECK(elementHarmony(Sign::Aries, Sign::Leo).find("Both Fire") == 0);
    CHECK(elementHarmony(Sign::Gemini, Sign::Leo).find("Fire and Air") == 0);
    CHECK(elementHarmony(Sign::Cancer, Sign::Taurus).find("Earth and Water") == 0);
    CHECK(elementHarmony(Sign::Aries, Sign::Cancer).find("different elements") != std::string::npos);
    CHECK(HoroscopeService::url(Sign::Leo, Period::Monthly) == "https://freehoroscopeapi.com/api/v1/get-horoscope/monthly?sign=Leo");
}

}  // namespace

int main() {
    dates();
    zodiac();
    readings();
    json();
    horoscopeCache();
    people();
    jsonParser();
    tarot();
    sky();
    extras();
    std::cout << g_passed << " checks passed, " << g_failed << " failed\n";
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
