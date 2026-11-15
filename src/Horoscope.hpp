#pragma once
// Real daily, weekly and monthly horoscopes from FreeHoroscopeAPI (https://freehoroscopeapi.com),
// cached on disk so the app works offline and only downloads when a new day/week starts.
#include <filesystem>
#include <optional>
#include <string>

#include "Date.hpp"
#include "Zodiac.hpp"

enum class Period { Daily, Weekly, Monthly };

struct Reading {
    Sign sign = Sign::Aries;
    Period period = Period::Weekly;
    std::string label;      // the provider's label: "2026-11-15" (day), "2026-11-09" (week start), "2026-11" (month)
    std::string text;
    Date fetchedOn;         // when we downloaded it
    bool fromCache = false; // shown from disk instead of the internet
    bool stale = false;     // from an earlier day/week because the download failed
};

struct HoroscopeResult {
    std::optional<Reading> reading;
    std::string problem;    // why a fresh reading couldn't be fetched (empty if it could)
};

class HoroscopeService {
public:
    explicit HoroscopeService(std::filesystem::path cacheDir);

    // Returns this day's/week's reading: from the cache if it's current, otherwise
    // downloaded now. If the download fails, the last saved reading is returned
    // with `stale` set, or none at all if there isn't one.
    HoroscopeResult get(Sign sign, Period period, const Date& today);

    static std::string url(Sign sign, Period period);
    static std::string path(Sign sign, Period period);  // "/api/v1/get-horoscope/weekly?sign=Leo"
    static const char* kAttribution;

private:
    std::filesystem::path file(Sign sign, Period period) const;
    std::optional<Reading> load(Sign sign, Period period) const;
    void save(const Reading& r) const;

    std::filesystem::path dir_;
};

const char* periodName(Period p);  // "daily" / "weekly" / "monthly"
