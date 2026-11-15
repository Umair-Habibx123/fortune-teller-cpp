#include "Readings.hpp"

#include <cstdlib>
#include <random>

#include "LegacyData.hpp"

std::vector<std::string> aboutYou(Sign sign, int birthDay) {
    const auto& notes = legacy::kAbout[static_cast<std::size_t>(index(sign))];
    int step = birthDay > 10 ? birthDay % 10 : birthDay;
    if (step == 0) step = 5;
    std::vector<std::string> picked;
    for (int i = 1; i <= 5; ++i) {
        const std::size_t at = static_cast<std::size_t>(i * step - 1);
        if (at >= notes.size()) break;  // the original read past the end here for short lists
        picked.push_back(notes[at]);
    }
    return picked;
}

std::vector<int> luckyNumbers(int birthDay) {
    const int x = birthDay;
    return {x, x + 7, x * 21 / 19, x + (x % 10)};
}

std::string fortuneCookie() {
    static std::mt19937 rng = [] {
        if (const char* seed = std::getenv("FORTUNE_SEED")) return std::mt19937(static_cast<unsigned>(std::strtoul(seed, nullptr, 10)));
        return std::mt19937(std::random_device{}());
    }();
    std::uniform_int_distribution<std::size_t> pick(0, legacy::kFortunes.size() - 1);
    return legacy::kFortunes[pick(rng)];
}

const std::string& compatibility(Sign a, Sign b) {
    return legacy::kLove[static_cast<std::size_t>(compatibilityIndex(a, b))];
}
