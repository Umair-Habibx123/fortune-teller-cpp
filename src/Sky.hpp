#pragma once
// Astronomy and Vedic calendar data from FreeHoroscopeAPI: planetary positions,
// Sun and Moon times (with Rahu Kaal), and the daily Panchang.
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Api.hpp"
#include "Date.hpp"

struct Location {
    std::string name;   // "Islamabad, Pakistan" or "Custom location"
    double lat = 0.0;
    double lng = 0.0;
};

// A short built-in list of major cities. Nothing is detected automatically.
const std::vector<Location>& knownCities();
std::optional<Location> findCity(const std::string& name);  // case-insensitive, also matches the start

struct Settings {
    std::optional<Location> location;
    bool sidereal = false;   // false = tropical (Western), true = sidereal (Vedic, approximate Lahiri)
    bool clock24 = true;
};
Settings loadSettings(const std::filesystem::path& file);
void saveSettings(const std::filesystem::path& file, const Settings& s);

struct Planet {
    std::string body;        // "sun", "moon", "mercury", ... "rahu", "ketu"
    std::string sign;        // "Scorpio"
    double degreesInSign = 0;
    bool retrograde = false;
    std::string nakshatra;   // sidereal only
    int pada = 0;            // sidereal only
};

struct Planets {
    std::string calculatedAt;  // UTC instant the positions are for
    std::vector<Planet> bodies;
};

Planets parsePlanets(const Json& doc);
std::string planetName(const std::string& body);  // "sun" -> "Sun", "rahu" -> "Rahu (north node)"

// "2026-11-15T06:35:10 +0500" -> "06:35" (24 h) or "6:35 AM" (12 h). Returns "-" if missing.
std::string clockTime(const std::string& isoLocal, bool clock24);
// "10h 42m"
std::string duration(long long seconds);

class Sky {
public:
    Sky(std::filesystem::path cacheDir, Settings settings);
    const Settings& settings() const { return settings_; }

    // Positions right now (refreshed once a day; the time they're for is shown).
    api::Result planetsNow(const Date& today) const;
    // Positions at noon UTC on a birthday (never change, so cached for good).
    api::Result planetsOn(const Date& birth, const Date& today) const;
    // Sun & Moon times and the Panchang for a date at the saved location.
    api::Result sunMoon(const Date& day, const Date& today) const;
    api::Result panchang(const Date& day, const Date& today) const;

private:
    std::string zodiac() const { return settings_.sidereal ? "sidereal" : "tropical"; }
    std::string place() const;  // "lat=..&lng=.." for the saved location
    std::filesystem::path dir_;
    Settings settings_;
};
