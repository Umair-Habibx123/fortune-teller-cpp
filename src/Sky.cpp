#include "Sky.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string coord(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.4f", v);
    return buf;
}

}  // namespace

const std::vector<Location>& knownCities() {
    static const std::vector<Location> cities = {
        {"Islamabad, Pakistan", 33.6844, 73.0479},   {"Rawalpindi, Pakistan", 33.5651, 73.0169},
        {"Lahore, Pakistan", 31.5204, 74.3587},      {"Karachi, Pakistan", 24.8607, 67.0011},
        {"Peshawar, Pakistan", 34.0151, 71.5249},    {"Quetta, Pakistan", 30.1798, 66.9750},
        {"Multan, Pakistan", 30.1575, 71.5249},      {"Faisalabad, Pakistan", 31.4504, 73.1350},
        {"Kathmandu, Nepal", 27.7172, 85.3240},      {"Delhi, India", 28.6139, 77.2090},
        {"Mumbai, India", 19.0760, 72.8777},         {"Dhaka, Bangladesh", 23.8103, 90.4125},
        {"Dubai, UAE", 25.2048, 55.2708},            {"Riyadh, Saudi Arabia", 24.7136, 46.6753},
        {"Makkah, Saudi Arabia", 21.3891, 39.8579},  {"Istanbul, Turkey", 41.0082, 28.9784},
        {"Cairo, Egypt", 30.0444, 31.2357},          {"Nairobi, Kenya", -1.2921, 36.8219},
        {"Lagos, Nigeria", 6.5244, 3.3792},          {"London, UK", 51.5074, -0.1278},
        {"Paris, France", 48.8566, 2.3522},          {"Berlin, Germany", 52.5200, 13.4050},
        {"Moscow, Russia", 55.7558, 37.6173},        {"New York, USA", 40.7128, -74.0060},
        {"Chicago, USA", 41.8781, -87.6298},         {"Los Angeles, USA", 34.0522, -118.2437},
        {"Toronto, Canada", 43.6532, -79.3832},      {"Mexico City, Mexico", 19.4326, -99.1332},
        {"Sao Paulo, Brazil", -23.5505, -46.6333},   {"Sydney, Australia", -33.8688, 151.2093},
        {"Tokyo, Japan", 35.6762, 139.6503},         {"Beijing, China", 39.9042, 116.4074},
        {"Singapore", 1.3521, 103.8198},             {"Jakarta, Indonesia", -6.2088, 106.8456},
        {"Kuala Lumpur, Malaysia", 3.1390, 101.6869},
    };
    return cities;
}

std::optional<Location> findCity(const std::string& name) {
    const std::string needle = lower(name);
    if (needle.empty()) return std::nullopt;
    for (const Location& c : knownCities())
        if (lower(c.name).rfind(needle, 0) == 0) return c;
    return std::nullopt;
}

// settings.txt: simple key=value lines
Settings loadSettings(const fs::path& file) {
    Settings s;
    std::ifstream in(file);
    std::string line;
    Location loc;
    bool hasLat = false, hasLng = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq), value = line.substr(eq + 1);
        try {
            if (key == "location_name") loc.name = value;
            else if (key == "lat") loc.lat = std::stod(value), hasLat = true;
            else if (key == "lng") loc.lng = std::stod(value), hasLng = true;
            else if (key == "zodiac") s.sidereal = value == "sidereal";
            else if (key == "clock") s.clock24 = value != "12";
        } catch (...) {
        }
    }
    if (hasLat && hasLng && loc.lat >= -90 && loc.lat <= 90 && loc.lng >= -180 && loc.lng <= 180) {
        if (loc.name.empty()) loc.name = "Custom location";
        s.location = loc;
    }
    return s;
}

void saveSettings(const fs::path& file, const Settings& s) {
    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);
    std::ofstream out(file, std::ios::trunc);
    if (s.location) out << "location_name=" << s.location->name << "\nlat=" << coord(s.location->lat) << "\nlng=" << coord(s.location->lng) << '\n';
    out << "zodiac=" << (s.sidereal ? "sidereal" : "tropical") << "\nclock=" << (s.clock24 ? "24" : "12") << '\n';
}

std::string planetName(const std::string& body) {
    if (body == "rahu") return "Rahu (north node)";
    if (body == "ketu") return "Ketu (south node)";
    std::string name = body;
    if (!name.empty()) name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    return name;
}

Planets parsePlanets(const Json& doc) {
    Planets p;
    p.calculatedAt = doc["datetime"].str();
    static const char* order[] = {"sun", "moon", "mercury", "venus", "mars", "jupiter", "saturn", "uranus", "neptune", "pluto", "rahu", "ketu"};
    const Json& bodies = doc["bodies"];
    for (const char* key : order) {
        const Json& b = bodies[key];
        if (!b.isObject()) continue;
        Planet planet;
        planet.body = key;
        planet.sign = b["sign"].str("?");
        planet.degreesInSign = b["degrees_in_sign"].num();
        planet.retrograde = b["retrograde"].boolean();
        planet.nakshatra = b["nakshatra"].str();
        planet.pada = static_cast<int>(b["pada"].num());
        p.bodies.push_back(planet);
    }
    return p;
}

std::string clockTime(const std::string& isoLocal, bool clock24) {
    const auto t = isoLocal.find('T');
    if (t == std::string::npos || t + 6 > isoLocal.size()) return "-";
    int hour = 0, minute = 0;
    if (std::sscanf(isoLocal.c_str() + t + 1, "%d:%d", &hour, &minute) != 2) return "-";
    // the API may already give 12-hour times with AM/PM
    const bool pm = isoLocal.find("PM") != std::string::npos;
    const bool am = isoLocal.find("AM") != std::string::npos;
    if (pm && hour < 12) hour += 12;
    if (am && hour == 12) hour = 0;
    char buf[16];
    if (clock24) {
        std::snprintf(buf, sizeof buf, "%02d:%02d", hour, minute);
    } else {
        const int h12 = hour % 12 == 0 ? 12 : hour % 12;
        std::snprintf(buf, sizeof buf, "%d:%02d %s", h12, minute, hour < 12 ? "AM" : "PM");
    }
    return buf;
}

std::string duration(long long seconds) {
    if (seconds < 0) seconds = -seconds;
    std::ostringstream out;
    out << seconds / 3600 << "h " << (seconds % 3600) / 60 << "m";
    return out.str();
}

Sky::Sky(fs::path cacheDir, Settings settings) : dir_(std::move(cacheDir)), settings_(std::move(settings)) {}

std::string Sky::place() const { return "lat=" + coord(settings_.location->lat) + "&lng=" + coord(settings_.location->lng); }

api::Result Sky::planetsNow(const Date& today) const {
    return api::cachedGet(dir_, "planets-now-" + zodiac(), "/api/v1/planetary-positions?zodiac=" + zodiac(), today,
                          [&](const Date& fetched) { return fetched == today; });
}

api::Result Sky::planetsOn(const Date& birth, const Date& today) const {
    return api::cachedGet(dir_, "planets-" + birth.iso() + "-" + zodiac(),
                          "/api/v1/planetary-positions?zodiac=" + zodiac() + "&datetime=" + birth.iso() + "T12:00:00Z", today,
                          [](const Date&) { return true; });
}

api::Result Sky::sunMoon(const Date& day, const Date& today) const {
    if (!settings_.location) return api::Result{std::nullopt, "no location chosen yet (Settings)"};
    // The answer for a given date and place never changes, so it's cached for good.
    return api::cachedGet(dir_, "times-" + day.iso() + "-" + coord(settings_.location->lat) + "_" + coord(settings_.location->lng),
                          "/api/v1/times?" + place() + "&date=" + day.iso() + "&timeformat=24&include=moon_details", today,
                          [](const Date&) { return true; });
}

api::Result Sky::panchang(const Date& day, const Date& today) const {
    if (!settings_.location) return api::Result{std::nullopt, "no location chosen yet (Settings)"};
    return api::cachedGet(dir_, "panchang-" + day.iso() + "-" + coord(settings_.location->lat) + "_" + coord(settings_.location->lng),
                          "/api/v1/panchang?" + place() + "&date=" + day.iso(), today, [](const Date&) { return true; });
}
