#include "Storage.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

fs::path dataDir() {
    if (const char* home = std::getenv("FORTUNE_TELLER_HOME"); home && *home) return fs::path(home);
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"); appdata && *appdata) return fs::path(appdata) / "FortuneTeller";
#else
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) return fs::path(xdg) / "fortune-teller";
    if (const char* home = std::getenv("HOME"); home && *home) return fs::path(home) / ".local" / "share" / "fortune-teller";
#endif
    return fs::current_path() / "fortune-teller-data";
}

PeopleBook::PeopleBook(fs::path file) : file_(std::move(file)) {
    std::ifstream in(file_);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream row(line);
        std::string name, birth, added;
        if (!std::getline(row, name, '\t') || !std::getline(row, birth, '\t')) continue;
        std::getline(row, added, '\t');
        auto b = parseDate(birth);
        if (name.empty() || !b) continue;
        auto a = parseDate(added);
        people_.push_back(Person{name, *b, a ? *a : *b});
    }
}

void PeopleBook::save() const {
    std::error_code ec;
    fs::create_directories(file_.parent_path(), ec);
    const fs::path temp = file_.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) return;
        for (const Person& p : people_) out << p.name << '\t' << p.birth.iso() << '\t' << p.added.iso() << '\n';
    }
    fs::rename(temp, file_, ec);
    if (ec) fs::remove(temp, ec);
}

void PeopleBook::add(const Person& p) {
    for (auto it = people_.begin(); it != people_.end(); ++it) {
        if (it->name == p.name && it->birth == p.birth) {
            people_.erase(it);
            break;
        }
    }
    people_.push_back(p);
    save();
}

bool PeopleBook::remove(std::size_t position) {
    if (position >= people_.size()) return false;
    people_.erase(people_.begin() + static_cast<std::ptrdiff_t>(position));
    save();
    return true;
}

std::vector<std::size_t> PeopleBook::bornOn(int month, int day) const {
    std::vector<std::size_t> hits;
    for (std::size_t i = 0; i < people_.size(); ++i)
        if (people_[i].birth.month == month && (day == 0 || people_[i].birth.day == day)) hits.push_back(i);
    return hits;
}

std::optional<Person> PeopleBook::lastUsed() const {
    if (people_.empty()) return std::nullopt;
    return people_.back();
}
