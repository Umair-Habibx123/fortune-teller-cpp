// Fortune Teller - a first-semester C project, rebuilt in modern C++17.
// Interactive by default; also has a few command-line shortcuts (see --help).
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "App.hpp"
#include "Console.hpp"
#include "Horoscope.hpp"
#include "Sky.hpp"
#include "Storage.hpp"
#include "Tarot.hpp"

namespace {

const char* kVersion = "2.1.0";

void usage() {
    std::cout << "Fortune Teller " << kVersion << "\n\n"
              << "Usage:\n"
              << "  fortune                       start the interactive program\n"
              << "  fortune --sign <sign>         print this week's horoscope for a sign\n"
              << "  fortune --birthday YYYY-MM-DD print the sign and weekly horoscope for a birthday\n"
              << "  fortune --sign <sign> --daily today's horoscope instead of this week's\n"
              << "  fortune --offline ...         don't use the internet (saved readings only)\n"
              << "  fortune --version | --help\n\n"
              << "Data is saved in: " << dataDir().string() << "\n"
              << HoroscopeService::kAttribution << "\n";
}

int printReading(Sign sign, Period period) {
    HoroscopeService service(dataDir() / "cache");
    const HoroscopeResult r = service.get(sign, period, today());
    std::cout << info(sign).name << " - " << periodName(period) << " horoscope";
    if (!r.reading) {
        std::cout << "\nNot available: " << r.problem << "\n";
        return 1;
    }
    std::cout << " (" << r.reading->label << ")\n\n";
    console::wrap(r.reading->text, "", 78);
    if (r.reading->stale) std::cout << "\n[saved reading from " << r.reading->fetchedOn.iso() << "; couldn't update: " << r.problem << "]\n";
    std::cout << "\n" << HoroscopeService::kAttribution << "\n";
    return 0;
}

int printTarot(int n) {
    Tarot tarot(dataDir() / "cache");
    const std::string problem = tarot.load(today());
    if (!tarot.ready()) {
        std::cout << "Tarot not available: " << problem << "\n";
        return 1;
    }
    for (const DrawnCard& c : tarot.draw(static_cast<std::size_t>(n))) {
        std::cout << c.card.name << (c.reversed ? " (reversed)" : " (upright)") << "\n";
        console::wrap(c.meaning(), "  ", 78);
        std::cout << "\n";
    }
    std::cout << HoroscopeService::kAttribution << "\n";
    return 0;
}

int printPlanets() {
    const Sky sky(dataDir() / "cache", loadSettings(dataDir() / "settings.txt"));
    const api::Result r = sky.planetsNow(today());
    if (!r.data) {
        std::cout << "Planetary positions not available: " << r.problem << "\n";
        return 1;
    }
    const Planets p = parsePlanets(r.data->json);
    std::cout << "Planets (" << (sky.settings().sidereal ? "sidereal" : "tropical") << ") at " << p.calculatedAt << " UTC\n\n";
    for (const Planet& b : p.bodies) {
        std::string name = planetName(b.body);
        name.resize(20, ' ');
        std::cout << "  " << name << b.sign << " " << static_cast<int>(b.degreesInSign) << " deg"
                  << (b.retrograde && b.body != "rahu" && b.body != "ketu" ? "  retrograde" : "") << "\n";
    }
    std::cout << "\n" << HoroscopeService::kAttribution << "\n";
    return 0;
}

int printSunMoon(const std::string& city) {
    Settings settings = loadSettings(dataDir() / "settings.txt");
    if (!city.empty()) {
        settings.location = findCity(city);
        if (!settings.location) {
            std::cerr << "Unknown city: " << city << ". Listed cities:\n";
            for (const Location& c : knownCities()) std::cerr << "  " << c.name << "\n";
            return 2;
        }
    }
    if (!settings.location) {
        std::cerr << "No city saved. Use --city <name>, or choose one in Settings in the interactive program.\n";
        return 2;
    }
    const Sky sky(dataDir() / "cache", settings);
    const api::Result r = sky.sunMoon(today(), today());
    if (!r.data) {
        std::cout << "Sun and Moon times not available: " << r.problem << "\n";
        return 1;
    }
    const Json& j = r.data->json;
    const bool c24 = settings.clock24;
    const Json& ev = j["sun"]["events"];
    std::cout << settings.location->name << ", " << today().pretty() << "\n\n"
              << "  Sunrise     " << clockTime(ev["sunrise"].str(), c24) << "\n"
              << "  Sunset      " << clockTime(ev["sunset"].str(), c24) << "\n"
              << "  Moonrise    " << clockTime(j["moon"]["moonrise"].str(), c24) << "\n"
              << "  Moonset     " << clockTime(j["moon"]["moonset"].str(), c24) << "\n"
              << "  Moon phase  " << j["moon"]["phase_description"].str("-") << "\n\n"
              << HoroscopeService::kAttribution << "\n";
    return 0;
}

#ifdef _WIN32
void setEnv(const char* name, const char* value) { _putenv_s(name, value); }
#else
void setEnv(const char* name, const char* value) { setenv(name, value, 1); }
#endif

}  // namespace

int main(int argc, char** argv) {
    console::setup();
    std::vector<std::string> args(argv + 1, argv + argc);

    Period period = Period::Weekly;
    std::string signArg, birthdayArg, cityArg;
    int tarotCards = 0;
    bool planets = false, sun = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--help" || a == "-h") {
            usage();
            return 0;
        } else if (a == "--version") {
            std::cout << "Fortune Teller " << kVersion << "\n";
            return 0;
        } else if (a == "--offline") {
            setEnv("FORTUNE_OFFLINE", "1");
        } else if (a == "--daily") {
            period = Period::Daily;
        } else if (a == "--monthly") {
            period = Period::Monthly;
        } else if (a == "--tarot") {
            tarotCards = 1;
            if (i + 1 < args.size() && !args[i + 1].empty() && std::isdigit(static_cast<unsigned char>(args[i + 1][0]))) {
                tarotCards = std::atoi(args[++i].c_str());
                if (tarotCards < 1 || tarotCards > 10) {
                    std::cerr << "--tarot takes 1 to 10 cards\n";
                    return 2;
                }
            }
        } else if (a == "--planets") {
            planets = true;
        } else if (a == "--sun") {
            sun = true;
        } else if (a == "--city" && i + 1 < args.size()) {
            cityArg = args[++i];
            sun = true;
        } else if (a == "--weekly") {
            period = Period::Weekly;
        } else if (a == "--sign" && i + 1 < args.size()) {
            signArg = args[++i];
        } else if (a == "--birthday" && i + 1 < args.size()) {
            birthdayArg = args[++i];
        } else {
            std::cerr << "Unknown option: " << a << "\n\n";
            usage();
            return 2;
        }
    }

    if (tarotCards > 0) return printTarot(tarotCards);
    if (planets) return printPlanets();
    if (sun) return printSunMoon(cityArg);
    if (!birthdayArg.empty()) {
        auto birth = parseDate(birthdayArg);
        if (!birth) {
            std::cerr << "Not a valid date: " << birthdayArg << " (use YYYY-MM-DD)\n";
            return 2;
        }
        const Sign s = signFor(*birth);
        std::cout << "Born " << birth->pretty() << ": " << info(s).name << " (" << info(s).title << ")\n\n";
        return printReading(s, period);
    }
    if (!signArg.empty()) {
        auto s = parseSign(signArg);
        if (!s) {
            std::cerr << "Unknown sign: " << signArg << "\n";
            return 2;
        }
        return printReading(*s, period);
    }

    App app;
    return app.run();
}
