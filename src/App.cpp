#include "App.hpp"

#include <cmath>
#include <cstdio>
#include <iostream>

#include "Console.hpp"
#include "LegacyData.hpp"
#include "Readings.hpp"

using namespace console;

namespace {

void banner() {
    // the crystal-ball face from the original welcome screen
    std::cout << accent("        \\WWWWW/") << '\n'
              << accent("       _|  o_o  |_") << "      " << bold("FORTUNE TELLER") << '\n'
              << accent("      (_   / \\   _)") << "     " << dim("want to know your future?") << '\n'
              << accent("        |  \\_/  |") << '\n'
              << accent("        : ~~~~~ :") << '\n'
              << accent("         \\_____/") << "\n\n";
}

void art(Sign s) {
    for (const std::string& line : legacy::kArt[static_cast<std::size_t>(index(s))]) std::cout << "    " << accent(line) << '\n';
}

void heading(const std::string& title, const std::string& note = "") {
    std::cout << '\n' << "  " << bold(title) << (note.empty() ? "" : dim("  (" + note + ")")) << '\n';
    rule();
}

void attribution() { std::cout << "  " << dim(HoroscopeService::kAttribution) << '\n'; }

// Shown under any live data that came from an older saved copy.
void staleNote(const api::Result& r) {
    if (r.data && r.data->stale)
        std::cout << "\n  " << warn("Couldn't update: " + r.problem + ". Showing the copy saved on " + r.data->fetchedOn.pretty() + ".") << '\n';
}

void unavailable(const std::string& what, const std::string& problem) {
    std::cout << "  " << warn(what + " isn't available right now: " + problem + ".") << '\n'
              << "  " << dim("Connect to the internet and try again; the classic readings work offline.") << '\n';
}

std::string pad(std::string s, std::size_t width) {
    if (s.size() < width) s.resize(width, ' ');
    return s;
}

std::string degrees(double d) {
    const int whole = static_cast<int>(d);
    const int minutes = static_cast<int>(std::round((d - whole) * 60));
    char buf[32];
    std::snprintf(buf, sizeof buf, "%2d\xC2\xB0 %02d'", minutes == 60 ? whole + 1 : whole, minutes == 60 ? 0 : minutes);
    return buf;
}

void printCard(const DrawnCard& c, const std::string& label = "") {
    std::cout << "  " << (label.empty() ? "" : dim(label + ": ")) << accent(c.card.name) << (c.reversed ? warn("  (reversed)") : dim("  (upright)")) << '\n';
    wrap(c.meaning(), "    ");
}

}  // namespace

App::App()
    : today_(today()),
      horoscopes_(dataDir() / "cache"),
      people_(dataDir() / "people.tsv"),
      tarot_(dataDir() / "cache"),
      sky_(dataDir() / "cache", loadSettings(dataDir() / "settings.txt")) {}

int App::run() {
    clear();
    banner();
    if (!chooseProfile()) return 0;
    while (home()) {
    }
    std::cout << "\n  " << dim("May the stars be kind to you. Goodbye!") << '\n';
    return 0;
}

// ---------------------------------------------------------------- who are you

bool App::chooseProfile() {
    if (auto last = people_.lastUsed()) {
        const Sign s = signFor(last->birth);
        auto answer = ask("  Welcome back, " + bold(last->name) + " (" + info(s).name + ")! Continue as " + last->name + "? [Y/n] ");
        if (!answer) return false;
        if (answer->empty() || (*answer)[0] == 'y' || (*answer)[0] == 'Y') {
            me_ = *last;
            sign_ = s;
            me_.added = today_;
            people_.add(me_);
            return true;
        }
    }
    return enterPerson("Enter your details");
}

bool App::enterPerson(const std::string& title) {
    me_ = Person{};
    heading(title);
    auto name = ask("  Full name: ");
    if (!name) return false;
    me_.name = name->empty() ? "Friend" : *name;

    for (;;) {
        auto entered = ask("  Date of birth (YYYY-MM-DD): ");
        if (!entered) return false;
        auto birth = parseDate(*entered);
        if (!birth) {
            std::cout << warn("  That isn't a real date. Example: 2002-05-14") << '\n';
            continue;
        }
        if (today_ < *birth) {
            std::cout << warn("  That date is in the future.") << '\n';
            continue;
        }
        if (ageOn(*birth, today_) > 120) {
            std::cout << warn("  Please check the year.") << '\n';
            continue;
        }
        me_.birth = *birth;
        break;
    }
    me_.added = today_;
    sign_ = signFor(me_.birth);
    people_.add(me_);
    return true;
}

void App::showSign() const {
    const SignInfo& s = info(sign_);
    const int untilBirthday = daysUntilBirthday(me_.birth, today_);
    std::cout << '\n'
              << "  " << bold(me_.name) << dim("  |  age " + std::to_string(ageOn(me_.birth, today_)) + "  |  born " + me_.birth.pretty())
              << "\n  "
              << (untilBirthday == 0 ? good("Happy birthday!") : dim(std::to_string(untilBirthday) + " days to your birthday"))
              << dim("  |  Chinese zodiac: " + chineseAnimal(me_.birth.year)) << "\n\n";
    art(sign_);
    std::cout << "\n  " << accent(std::string(s.name) + " - " + s.title) << dim("   " + std::string(s.dates)) << '\n'
              << "  " << dim("Element: ") << s.element << dim("   Ruling planet: ") << s.planet << '\n';
}

// ---------------------------------------------------------------- horoscopes

void App::showHoroscope(Period period, bool compact) {
    const HoroscopeResult r = horoscopes_.get(sign_, period, today_);
    const char* titles[] = {"Today's horoscope", "This week's horoscope", "This month's horoscope"};
    const std::string title = titles[static_cast<int>(period)];
    if (!r.reading) {
        heading(title);
        std::cout << "  " << warn("No horoscope available right now: " + r.problem + ".") << '\n'
                  << "  " << dim("Connect to the internet and try again; everything else works offline.") << '\n';
        return;
    }
    const Reading& reading = *r.reading;
    heading(title, (period == Period::Weekly ? "week of " : "") + reading.label);
    wrap(reading.text);
    if (reading.stale) {
        std::cout << "\n  " << warn("Couldn't download a new one: " + r.problem + ".") << '\n'
                  << "  " << warn("Showing your last saved reading, from " + reading.fetchedOn.pretty() + ".") << '\n';
    } else if (!compact) {
        std::cout << "\n  " << dim(reading.fromCache ? "Saved on " + reading.fetchedOn.pretty() + ", updates automatically." : "Just downloaded.")
                  << '\n';
    }
    attribution();
}

// ---------------------------------------------------------------- tarot

void App::showCardOfTheDay() {
    const std::string problem = tarot_.load(today_);
    heading("Your tarot card of the day", today_.pretty());
    if (!tarot_.ready()) return unavailable("Tarot", problem);
    printCard(tarot_.cardOfTheDay(me_.name + me_.birth.iso(), today_));
    std::cout << "\n  " << dim("The same card stays with you all day.") << '\n';
    attribution();
}

void App::showSpread() {
    const std::string problem = tarot_.load(today_);
    heading("Three-card spread", "past, present, future");
    if (!tarot_.ready()) return unavailable("Tarot", problem);
    const char* labels[] = {"Past", "Present", "Future"};
    const auto cards = tarot_.draw(3);
    for (std::size_t i = 0; i < cards.size(); ++i) {
        printCard(cards[i], labels[i]);
        std::cout << '\n';
    }
    attribution();
}

bool App::browseDeck() {
    const std::string problem = tarot_.load(today_);
    heading("The tarot deck", std::to_string(tarot_.deck().size()) + " cards");
    if (!tarot_.ready()) {
        unavailable("Tarot", problem);
        return true;
    }
    std::cout << "  1. Major arcana   2. Wands   3. Cups   4. Swords   5. Pentacles   6. Search by name\n";
    auto choice = askNumber("\n  Choose: ");
    if (!choice) return false;
    std::vector<TarotCard> cards;
    const char* groups[] = {"", "major", "wands", "cups", "swords", "pentacles"};
    if (*choice >= 1 && *choice <= 5) {
        cards = tarot_.group(groups[*choice]);
    } else if (*choice == 6) {
        auto text = ask("  Card name (e.g. tower, queen of cups): ");
        if (!text) return false;
        cards = tarot_.search(*text);
    } else {
        std::cout << warn("  Please choose 1-6.") << '\n';
        return true;
    }
    if (cards.empty()) {
        std::cout << "  " << dim("No cards found.") << '\n';
        return true;
    }
    if (cards.size() > 1) {
        for (std::size_t i = 0; i < cards.size(); ++i)
            std::cout << "  " << (i + 1 < 10 ? " " : "") << i + 1 << ". " << cards[i].name << '\n';
        auto pick = askNumber("\n  Read which card? (0 = none): ");
        if (!pick) return false;
        if (*pick < 1 || static_cast<std::size_t>(*pick) > cards.size()) return true;
        cards = {cards[static_cast<std::size_t>(*pick - 1)]};
    }
    const TarotCard& c = cards.front();
    heading(c.name, c.arcana == "major" ? "major arcana" : c.suit);
    std::cout << "  " << bold("Upright") << '\n';
    wrap(c.meaningUp, "    ");
    std::cout << "  " << bold("Reversed") << '\n';
    wrap(c.meaningReversed, "    ");
    if (!c.description.empty()) {
        std::cout << "  " << bold("The card") << '\n';
        wrap(c.description, "    ");
    }
    attribution();
    return true;
}

// ---------------------------------------------------------------- sky

void App::showPlanets(bool atBirth) {
    const api::Result r = atBirth ? sky_.planetsOn(me_.birth, today_) : sky_.planetsNow(today_);
    const bool sidereal = sky_.settings().sidereal;
    heading(atBirth ? "The planets on your birthday" : "The planets right now", sidereal ? "sidereal / Vedic" : "tropical / Western");
    if (!r.data) return unavailable("Planetary positions", r.problem);
    const Planets p = parsePlanets(r.data->json);
    for (const Planet& b : p.bodies) {
        std::cout << "  " << pad(planetName(b.body), 20) << pad(b.sign, 13) << degrees(b.degreesInSign)
                  << (b.retrograde && b.body != "rahu" && b.body != "ketu" ? warn("  retrograde") : "");
        if (sidereal && !b.nakshatra.empty()) std::cout << dim("  " + b.nakshatra + " pada " + std::to_string(b.pada));
        std::cout << '\n';
    }
    if (atBirth) {
        std::cout << "\n  " << dim("Calculated for noon (UTC) on " + me_.birth.pretty() + ". Your Moon sign may differ if you were") << '\n'
                  << "  " << dim("born close to midnight; the other planets barely move in a day.") << '\n';
    } else {
        std::cout << "\n  " << dim("Positions for " + p.calculatedAt + " (UTC). Rahu and Ketu always move backwards.") << '\n';
    }
    staleNote(r);
    attribution();
}

void App::showSunMoon() {
    const auto& loc = sky_.settings().location;
    heading("Sun and Moon today", loc ? loc->name : "no location");
    if (!loc) {
        std::cout << "  " << dim("Choose your city in Settings first (option 15).") << '\n';
        return;
    }
    const api::Result r = sky_.sunMoon(today_, today_);
    if (!r.data) return unavailable("Sun and Moon times", r.problem);
    const Json& j = r.data->json;
    const bool c24 = sky_.settings().clock24;
    const Json& sun = j["sun"];
    const Json& events = sun["events"];
    std::cout << "  " << pad("Sunrise", 16) << clockTime(events["sunrise"].str(), c24) << "      " << pad("Sunset", 10)
              << clockTime(events["sunset"].str(), c24) << '\n'
              << "  " << pad("Dawn", 16) << clockTime(events["dawn"].str(), c24) << "      " << pad("Dusk", 10)
              << clockTime(events["dusk"].str(), c24) << '\n'
              << "  " << pad("Solar noon", 16) << clockTime(events["noon"].str(), c24) << '\n';
    const Json& day = sun["daylight"];
    if (day["duration_seconds"].type() == Json::Type::Number) {
        const long long change = static_cast<long long>(day["change_from_yesterday_seconds"].num());
        std::cout << "  " << pad("Daylight", 16) << duration(static_cast<long long>(day["duration_seconds"].num()))
                  << dim("  (" + std::string(change < 0 ? "-" : "+") + std::to_string(change < 0 ? -change : change) + " s vs yesterday)") << '\n';
    }
    const Json& golden = sun["photography"]["golden_hour"];
    if (golden.isObject())
        std::cout << "  " << pad("Golden hour", 16) << clockTime(golden["morning"]["start"].str(), c24) << "-"
                  << clockTime(golden["morning"]["end"].str(), c24) << dim(" and ") << clockTime(golden["evening"]["start"].str(), c24)
                  << "-" << clockTime(golden["evening"]["end"].str(), c24) << '\n';

    const Json& moon = j["moon"];
    std::cout << '\n'
              << "  " << pad("Moonrise", 16) << clockTime(moon["moonrise"].str(), c24) << "      " << pad("Moonset", 10)
              << clockTime(moon["moonset"].str(), c24) << '\n'
              << "  " << pad("Moon phase", 16) << moon["phase_description"].str("-");
    const Json& details = moon["details"];
    if (details["illumination_pct"].type() == Json::Type::Number) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "  (%.0f%% lit)", details["illumination_pct"].num());
        std::cout << dim(buf);
    }
    std::cout << '\n';
    const Json& rahu = j["rahukaalam"];
    if (rahu.isObject())
        std::cout << "\n  " << pad("Rahu Kaal", 16) << clockTime(rahu["start"].str(), c24) << "-" << clockTime(rahu["end"].str(), c24)
                  << dim("  (a period traditionally avoided for new starts)") << '\n';
    std::cout << "  " << dim("Times in " + j["timezone"].str("local time") + ".") << '\n';
    staleNote(r);
    attribution();
}

void App::showPanchang() {
    const auto& loc = sky_.settings().location;
    heading("Panchang today", loc ? loc->name : "no location");
    if (!loc) {
        std::cout << "  " << dim("Choose your city in Settings first (option 15).") << '\n';
        return;
    }
    const api::Result r = sky_.panchang(today_, today_);
    if (!r.data) return unavailable("The Panchang", r.problem);
    const Json& j = r.data->json;
    const bool c24 = sky_.settings().clock24;
    std::cout << "  " << pad("Weekday", 12) << j["weekday"].str("-") << '\n';
    auto part = [&](const char* key, const char* label) {
        const Json& p = j[key];
        const Json& start = p["at_start"];
        std::string value = start["name"].str("-");
        if (start.has("paksha")) value += " (" + start["paksha"].str() + " paksha)";
        std::cout << "  " << pad(label, 12) << value;
        const Json& next = p["transitions"];
        if (next.isArray() && next.size() > 0)
            std::cout << dim("  -> " + next[0]["to"]["name"].str() + " from " + clockTime(next[0]["datetime"].str(), c24));
        std::cout << '\n';
    };
    part("tithi", "Tithi");
    part("nakshatra", "Nakshatra");
    part("yoga", "Yoga");
    part("karana", "Karana");
    std::cout << "\n  " << dim("Values at sunrise, with the next change. Day runs sunrise to sunrise; times in " + j["timezone"].str("local time") + ".")
              << '\n';
    staleNote(r);
    attribution();
}

bool App::chooseLocation() {
    heading("Choose your city");
    const auto& cities = knownCities();
    for (std::size_t i = 0; i < cities.size(); i += 2) {
        std::cout << "  " << (i + 1 < 10 ? " " : "") << i + 1 << ". " << pad(cities[i].name, 26);
        if (i + 1 < cities.size()) std::cout << (i + 2 < 10 ? " " : "") << i + 2 << ". " << cities[i + 1].name;
        std::cout << '\n';
    }
    std::cout << "  " << cities.size() + 1 << ". Somewhere else (type latitude and longitude)\n";
    auto choice = askNumber("\n  Choose: ");
    if (!choice) return false;
    Settings s = sky_.settings();
    if (*choice >= 1 && static_cast<std::size_t>(*choice) <= cities.size()) {
        s.location = cities[static_cast<std::size_t>(*choice - 1)];
    } else if (static_cast<std::size_t>(*choice) == cities.size() + 1) {
        auto lat = ask("  Latitude (-90 to 90, e.g. 33.68): ");
        if (!lat) return false;
        auto lng = ask("  Longitude (-180 to 180, e.g. 73.05): ");
        if (!lng) return false;
        try {
            const double la = std::stod(*lat), lo = std::stod(*lng);
            if (la < -90 || la > 90 || lo < -180 || lo > 180) throw 1;
            s.location = Location{"Custom location", la, lo};
        } catch (...) {
            std::cout << warn("  Those aren't valid coordinates.") << '\n';
            return true;
        }
    } else {
        std::cout << warn("  Please choose from the list.") << '\n';
        return true;
    }
    saveSettings(dataDir() / "settings.txt", s);
    sky_ = Sky(dataDir() / "cache", s);
    std::cout << good("  Saved: " + s.location->name) << '\n';
    return true;
}

bool App::settingsMenu() {
    for (;;) {
        const Settings& s = sky_.settings();
        heading("Settings", "saved on this computer");
        std::cout << "  1. Location        " << (s.location ? s.location->name : dim("not set")) << '\n'
                  << "  2. Zodiac system   " << (s.sidereal ? "Sidereal (Vedic)" : "Tropical (Western)") << '\n'
                  << "  3. Clock           " << (s.clock24 ? "24-hour" : "12-hour") << '\n'
                  << "  4. Back\n";
        auto choice = askNumber("\n  Choose: ");
        if (!choice) return false;
        Settings next = s;
        if (*choice == 1) {
            if (!chooseLocation()) return false;
            continue;
        }
        if (*choice == 2) next.sidereal = !next.sidereal;
        else if (*choice == 3) next.clock24 = !next.clock24;
        else if (*choice == 4) return true;
        else continue;
        saveSettings(dataDir() / "settings.txt", next);
        sky_ = Sky(dataDir() / "cache", next);
    }
}

// ---------------------------------------------------------------- classic readings

void App::showAbout() const {
    heading("Here is what you are");
    for (const std::string& note : aboutYou(sign_, me_.birth.day)) {
        wrap("* " + note);
        std::cout << '\n';
    }
}

void App::showFortune() const {
    heading("Your fortune for today");
    wrap(fortuneCookie());
}

void App::showLucky() const {
    heading("Your lucky numbers");
    std::cout << "  ";
    for (int n : luckyNumbers(me_.birth.day)) std::cout << accent(std::to_string(n)) << "    ";
    std::cout << '\n';
}

bool App::showCompatibility() const {
    heading("Love compatibility", "pick your partner's sign");
    for (int i = 0; i < kSignCount; i += 2) {
        std::cout << "  " << pad(std::to_string(i + 1) + ". " + info(signAt(i)).name, 22) << std::to_string(i + 2) << ". " << info(signAt(i + 1)).name
                  << '\n';
    }
    auto choice = askNumber("\n  Partner's sign (1-12): ");
    if (!choice) return false;
    if (*choice < 1 || *choice > kSignCount) {
        std::cout << warn("  There are only 12 signs.") << '\n';
        return true;
    }
    const Sign partner = signAt(*choice - 1);
    heading(std::string(info(sign_).name) + " + " + info(partner).name);
    std::cout << "  " << accent(elementHarmony(sign_, partner)) << "\n\n";
    wrap(compatibility(sign_, partner));
    return true;
}

bool App::peopleBook() {
    for (;;) {
        const auto& all = people_.all();
        heading("People book", std::to_string(all.size()) + " saved on this computer");
        std::cout << "  1. Everyone        2. Search birthdays        3. Delete someone        4. Back\n";
        auto choice = askNumber("\n  Choose: ");
        if (!choice) return false;
        if (*choice == 1 || *choice == 2) {
            std::vector<std::size_t> rows;
            if (*choice == 2) {
                auto month = askNumber("  Month (1-12): ");
                if (!month) return false;
                auto day = askNumber("  Day (0 = whole month): ");
                if (!day) return false;
                rows = people_.bornOn(*month, *day);
            } else {
                for (std::size_t i = 0; i < all.size(); ++i) rows.push_back(i);
            }
            if (rows.empty()) std::cout << "  " << dim("Nobody found.") << '\n';
            for (std::size_t i : rows) {
                const Person& p = all[i];
                std::cout << "  " << (i + 1 < 10 ? " " : "") << i + 1 << ". " << pad(p.name, 26) << ' ' << p.birth.iso() << "  "
                          << info(signFor(p.birth)).name << dim("  age " + std::to_string(ageOn(p.birth, today_))) << '\n';
            }
        } else if (*choice == 3) {
            auto which = askNumber("  Number to delete (0 = cancel): ");
            if (!which) return false;
            if (*which == 0) continue;
            if (*which >= 1 && people_.remove(static_cast<std::size_t>(*which - 1))) std::cout << good("  Deleted.") << '\n';
            else std::cout << warn("  No one with that number.") << '\n';
        } else if (*choice == 4) {
            return true;
        }
    }
}

// ---------------------------------------------------------------- home

bool App::home() {
    clear();
    showSign();
    showHoroscope(Period::Weekly, true);  // downloaded automatically when a new week starts

    heading("What would you like to see?");
    std::cout << "  " << dim("Horoscopes") << "      1. This week     2. Today         3. This month\n"
              << "  " << dim("Tarot") << "           4. Card of the day   5. Past-present-future   6. Browse the deck\n"
              << "  " << dim("Sky") << "             7. Planets now   8. Your birth planets   9. Sun & Moon   10. Panchang\n"
              << "  " << dim("Classic (2018)") << "  11. About you   12. Fortune cookie   13. Lucky numbers   14. Love match\n"
              << "  " << dim("More") << "            15. Settings    16. People book    17. Someone else    0. Exit\n";
    auto choice = askNumber("\n  Choose: ");
    if (!choice) return false;
    switch (*choice) {
        case 1: showHoroscope(Period::Weekly); break;
        case 2: showHoroscope(Period::Daily); break;
        case 3: showHoroscope(Period::Monthly); break;
        case 4: showCardOfTheDay(); break;
        case 5: showSpread(); break;
        case 6:
            if (!browseDeck()) return false;
            break;
        case 7: showPlanets(false); break;
        case 8: showPlanets(true); break;
        case 9: showSunMoon(); break;
        case 10: showPanchang(); break;
        case 11: showAbout(); break;
        case 12: showFortune(); break;
        case 13: showLucky(); break;
        case 14:
            if (!showCompatibility()) return false;
            break;
        case 15: return settingsMenu();
        case 16: return peopleBook();
        case 17: return enterPerson("Enter their details");
        case 0: return false;
        default: std::cout << warn("  Please choose a number from the menu.") << '\n'; break;
    }
    pause();
    return true;
}
