#include "Horoscope.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include "Api.hpp"
#include "Json.hpp"

namespace fs = std::filesystem;

const char* HoroscopeService::kAttribution = "Powered by FreeHoroscopeAPI - https://freehoroscopeapi.com";

const char* periodName(Period p) {
    switch (p) {
        case Period::Daily: return "daily";
        case Period::Weekly: return "weekly";
        case Period::Monthly: return "monthly";
    }
    return "daily";
}

namespace {

// A cached reading is current if it was fetched today (daily), this ISO week
// (weekly) or this calendar month (monthly).
bool isCurrent(const Reading& r, const Date& today) {
    switch (r.period) {
        case Period::Daily: return r.fetchedOn == today;
        case Period::Weekly: return isoWeek(r.fetchedOn) == isoWeek(today);
        case Period::Monthly: return r.fetchedOn.year == today.year && r.fetchedOn.month == today.month;
    }
    return false;
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
    return std::string(api::kBase) + path(sign, period);
}

std::string HoroscopeService::path(Sign sign, Period period) {
    return std::string("/api/v1/get-horoscope/") + periodName(period) + "?sign=" + info(sign).name;
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

    // Not cached for this day/week/month yet: download a fresh one.
    const api::Response fresh = api::get(path(sign, period));
    if (fresh.json) {
        const Json& data = (*fresh.json)["data"];
        const std::string text = trim(data["horoscope"].str());
        if (!text.empty()) {
            Reading reading;
            reading.sign = sign;
            reading.period = period;
            reading.label = data["date"].str(today.iso());
            reading.text = text;
            reading.fetchedOn = today;
            save(reading);
            result.reading = reading;
            return result;
        }
        result.problem = "the horoscope service sent an answer without a reading";
    } else {
        result.problem = fresh.problem;
    }

    // Couldn't download: fall back to the last saved reading, clearly marked as old.
    if (cached) {
        cached->stale = true;
        result.reading = cached;
    }
    return result;
}
