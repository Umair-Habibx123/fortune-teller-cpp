#include "Horoscope.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include "Http.hpp"
#include "Json.hpp"

namespace fs = std::filesystem;

const char* HoroscopeService::kAttribution = "Powered by FreeHoroscopeAPI - https://freehoroscopeapi.com";

const char* periodName(Period p) { return p == Period::Daily ? "daily" : "weekly"; }

namespace {

// A cached reading is current if it was fetched today (daily) or this ISO week (weekly).
bool isCurrent(const Reading& r, const Date& today) {
    return r.period == Period::Daily ? r.fetchedOn == today : isoWeek(r.fetchedOn) == isoWeek(today);
}

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    return s.substr(start);
}

}  // namespace

HoroscopeService::HoroscopeService(fs::path cacheDir) : dir_(std::move(cacheDir)) {}

std::string HoroscopeService::url(Sign sign, Period period) {
    return std::string("https://freehoroscopeapi.com/api/v1/get-horoscope/") + periodName(period) +
           "?sign=" + info(sign).name;
}

fs::path HoroscopeService::file(Sign sign, Period period) const {
    std::string name = info(sign).name;
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return dir_ / (std::string(periodName(period)) + "-" + name + ".txt");
}

// Cache file format (plain text, easy to inspect):
//   line 1: date we fetched it (YYYY-MM-DD)
//   line 2: the provider's date label
//   rest:   the reading
std::optional<Reading> HoroscopeService::load(Sign sign, Period period) const {
    std::ifstream in(file(sign, period), std::ios::binary);
    if (!in) return std::nullopt;
    std::string fetched, label;
    if (!std::getline(in, fetched) || !std::getline(in, label)) return std::nullopt;
    std::stringstream rest;
    rest << in.rdbuf();
    auto date = parseDate(trim(fetched));
    const std::string text = trim(rest.str());
    if (!date || text.empty()) return std::nullopt;
    Reading r;
    r.sign = sign;
    r.period = period;
    r.label = trim(label);
    r.text = text;
    r.fetchedOn = *date;
    r.fromCache = true;
    return r;
}

void HoroscopeService::save(const Reading& r) const {
    std::error_code ec;
    fs::create_directories(dir_, ec);
    const fs::path target = file(r.sign, r.period);
    const fs::path temp = target.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return;  // caching is best-effort
        out << r.fetchedOn.iso() << '\n' << r.label << '\n' << r.text << '\n';
    }
    fs::rename(temp, target, ec);
    if (ec) fs::remove(temp, ec);
}

HoroscopeResult HoroscopeService::get(Sign sign, Period period, const Date& today) {
    HoroscopeResult result;
    std::optional<Reading> cached = load(sign, period);
    if (cached && isCurrent(*cached, today)) {
        result.reading = cached;
        return result;
    }

    // Not cached for this day/week yet: download a fresh one.
    const HttpResult http = httpGet(url(sign, period));
    if (http.ok()) {
        auto text = jsonString(http.body, "horoscope");
        auto label = jsonString(http.body, "date");
        if (text && !trim(*text).empty()) {
            Reading fresh;
            fresh.sign = sign;
            fresh.period = period;
            fresh.label = label ? *label : today.iso();
            fresh.text = trim(*text);
            fresh.fetchedOn = today;
            save(fresh);
            result.reading = fresh;
            return result;
        }
        result.problem = "the horoscope service sent an answer without a reading";
    } else if (http.status != 0) {
        auto message = jsonString(http.body, "error");
        result.problem = "the horoscope service answered HTTP " + std::to_string(http.status) + (message ? ": " + *message : "");
    } else {
        result.problem = "no internet connection (" + http.error + ")";
    }

    // Couldn't download: fall back to the last saved reading, clearly marked as old.
    if (cached) {
        cached->stale = true;
        result.reading = cached;
    }
    return result;
}
