#pragma once
// Everything the app saves lives in one folder on this computer:
//   Windows: %APPDATA%\FortuneTeller     macOS/Linux: ~/.local/share/fortune-teller
//   (or FORTUNE_TELLER_HOME if set, which the tests use)
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Date.hpp"

struct Person {
    std::string name;
    Date birth;
    Date added;
};

std::filesystem::path dataDir();

// The "people" book: everyone who got a reading on this computer (the old
// "user's directory" from the administrator tools). Saved as tab-separated text.
class PeopleBook {
public:
    explicit PeopleBook(std::filesystem::path file);

    const std::vector<Person>& all() const { return people_; }
    void add(const Person& p);                     // replaces an existing entry with the same name + birthday
    bool remove(std::size_t position);             // 0-based
    std::vector<std::size_t> bornOn(int month, int day) const;
    std::optional<Person> lastUsed() const;        // most recently added, to greet returning users

private:
    void save() const;
    std::filesystem::path file_;
    std::vector<Person> people_;
};
