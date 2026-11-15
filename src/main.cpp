// Fortune Teller - a first-semester C project, rebuilt in modern C++17.
// Interactive by default; also has a few command-line shortcuts (see --help).
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "App.hpp"
#include "Console.hpp"
#include "Horoscope.hpp"
#include "Storage.hpp"

namespace {

const char* kVersion = "2.0.0";

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
    std::cout << info(sign).name << " - " << (period == Period::Weekly ? "weekly" : "daily") << " horoscope";
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
    std::string signArg, birthdayArg;
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
