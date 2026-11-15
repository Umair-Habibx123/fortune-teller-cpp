#include "App.hpp"

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

}  // namespace

App::App() : today_(today()), horoscopes_(dataDir() / "cache"), people_(dataDir() / "people.tsv") {}

int App::run() {
    clear();
    banner();
    if (!chooseProfile()) return 0;
    while (home()) {
    }
    std::cout << "\n  " << dim("May the stars be kind to you. Goodbye!") << '\n';
    return 0;
}

bool App::chooseProfile() {
    if (auto last = people_.lastUsed()) {
        const Sign s = signFor(last->birth);
        auto answer = ask("  Welcome back, " + bold(last->name) + " (" + info(s).name + ")! Continue as " + last->name +
                          "? [Y/n] ");
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

bool App::enterPerson(const std::string& heading) {
    me_ = Person{};
    std::cout << '\n' << "  " << bold(heading) << '\n';
    rule();
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
    std::cout << '\n' << "  " << bold(me_.name) << dim("  |  age " + std::to_string(ageOn(me_.birth, today_)) + "  |  born " + me_.birth.pretty()) << "\n\n";
    art(sign_);
    std::cout << "\n  " << accent(std::string(s.name) + " - " + s.title) << dim("   " + std::string(s.dates)) << '\n'
              << "  " << dim("Element: ") << s.element << dim("   Ruling planet: ") << s.planet << '\n';
}

void App::showHoroscope(Period period, bool compact) {
    const HoroscopeResult r = horoscopes_.get(sign_, period, today_);
    const std::string title = period == Period::Weekly ? "This week's horoscope" : "Today's horoscope";
    std::cout << '\n' << "  " << bold(title);
    if (r.reading) {
        const Reading& reading = *r.reading;
        std::cout << dim("  (" + std::string(period == Period::Weekly ? "week of " : "") + reading.label + ")") << '\n';
        rule();
        wrap(reading.text);
        if (reading.stale) {
            std::cout << "\n  " << warn("Couldn't download a new one: " + r.problem + ".") << '\n'
                      << "  " << warn("Showing your last saved reading, from " + reading.fetchedOn.pretty() + ".") << '\n';
        } else if (!compact) {
            std::cout << "\n  " << dim(reading.fromCache ? "Saved on " + reading.fetchedOn.pretty() + ", updates automatically."
                                                         : "Just downloaded.")
                      << '\n';
        }
        std::cout << "  " << dim(HoroscopeService::kAttribution) << '\n';
    } else {
        std::cout << '\n';
        rule();
        std::cout << "  " << warn("No horoscope available right now: " + r.problem + ".") << '\n'
                  << "  " << dim("Connect to the internet and try again; everything else works offline.") << '\n';
    }
}

void App::showAbout() const {
    std::cout << '\n' << "  " << bold("Here is what you are") << '\n';
    rule();
    for (const std::string& note : aboutYou(sign_, me_.birth.day)) {
        wrap("* " + note);
        std::cout << '\n';
    }
}

void App::showFortune() const {
    std::cout << '\n' << "  " << bold("Your fortune for today") << '\n';
    rule();
    wrap(fortuneCookie());
}

void App::showLucky() const {
    std::cout << '\n' << "  " << bold("Your lucky numbers") << '\n';
    rule();
    std::cout << "  ";
    for (int n : luckyNumbers(me_.birth.day)) std::cout << accent(std::to_string(n)) << "    ";
    std::cout << '\n';
}

bool App::showCompatibility() const {
    std::cout << '\n' << "  " << bold("Love compatibility") << " - pick your partner's sign" << '\n';
    rule();
    for (int i = 0; i < kSignCount; i += 2) {
        std::string left = std::to_string(i + 1) + ". " + info(signAt(i)).name;
        left.resize(22, ' ');
        std::cout << "  " << left << std::to_string(i + 2) << ". " << info(signAt(i + 1)).name << '\n';
    }
    auto choice = askNumber("\n  Partner's sign (1-12): ");
    if (!choice) return false;
    if (*choice < 1 || *choice > kSignCount) {
        std::cout << warn("  There are only 12 signs.") << '\n';
        return true;
    }
    const Sign partner = signAt(*choice - 1);
    std::cout << '\n' << "  " << accent(std::string(info(sign_).name) + " + " + info(partner).name) << '\n';
    rule();
    wrap(compatibility(sign_, partner));
    return true;
}

bool App::peopleBook() {
    for (;;) {
        const auto& all = people_.all();
        std::cout << '\n' << "  " << bold("People book") << dim("  (" + std::to_string(all.size()) + " saved on this computer)") << '\n';
        rule();
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
                std::string name = p.name;
                if (name.size() < 26) name.resize(26, ' ');
                std::cout << "  " << (i + 1 < 10 ? " " : "") << i + 1 << ". " << name << ' ' << p.birth.iso() << "  "
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

bool App::home() {
    clear();
    showSign();
    showHoroscope(Period::Weekly, true);  // downloaded automatically when a new week starts

    std::cout << '\n' << "  " << bold("What would you like to see?") << '\n';
    rule();
    std::cout << "  1. This week's horoscope      2. Today's horoscope\n"
                 "  3. About you                  4. Fortune cookie\n"
                 "  5. Lucky numbers              6. Love compatibility\n"
                 "  7. People book                8. Someone else\n"
                 "  0. Exit\n";
    auto choice = askNumber("\n  Choose: ");
    if (!choice) return false;
    switch (*choice) {
        case 1: showHoroscope(Period::Weekly); break;
        case 2: showHoroscope(Period::Daily); break;
        case 3: showAbout(); break;
        case 4: showFortune(); break;
        case 5: showLucky(); break;
        case 6:
            if (!showCompatibility()) return false;
            break;
        case 7:
            if (!peopleBook()) return false;
            return true;
        case 8: return enterPerson("Enter their details");
        case 0: return false;
        default: std::cout << warn("  Please choose a number from the menu.") << '\n'; break;
    }
    pause();
    return true;
}
