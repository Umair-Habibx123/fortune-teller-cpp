#include "Tarot.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <random>

#include "Api.hpp"

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::mt19937& rng() {
    static std::mt19937 engine = [] {
        if (const char* seed = std::getenv("FORTUNE_SEED")) return std::mt19937(static_cast<unsigned>(std::strtoul(seed, nullptr, 10)));
        return std::mt19937(std::random_device{}());
    }();
    return engine;
}

// FNV-1a: a stable hash, so the card of the day doesn't change between runs.
unsigned long long stableHash(const std::string& s) {
    unsigned long long h = 1469598103934665603ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

}  // namespace

Tarot::Tarot(std::filesystem::path cacheDir) : dir_(std::move(cacheDir)) {}

std::vector<TarotCard> Tarot::parseDeck(const Json& doc) {
    std::vector<TarotCard> cards;
    for (const Json& c : doc["cards"].items()) {
        TarotCard card;
        card.name = c["name"].str();
        card.shortName = c["name_short"].str();
        card.arcana = c["type"].str();
        card.suit = c["suit"].str();
        card.value = static_cast<int>(c["value_int"].num());
        card.meaningUp = c["meaning_up"].str();
        card.meaningReversed = c["meaning_rev"].str();
        card.description = c["desc"].str();
        if (!card.name.empty()) cards.push_back(std::move(card));
    }
    return cards;
}

std::string Tarot::load(const Date& today) {
    if (!deck_.empty()) return {};
    // the deck never changes, so a saved copy is always good
    const api::Result r = api::cachedGet(dir_, "tarot-deck", "/api/v1/tarot/cards", today, [](const Date&) { return true; });
    if (r.data) deck_ = parseDeck(r.data->json);
    if (deck_.empty()) return r.problem.empty() ? "the tarot deck couldn't be read" : r.problem;
    return {};
}

std::vector<DrawnCard> Tarot::draw(std::size_t n) const {
    std::vector<std::size_t> order(deck_.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng());
    std::bernoulli_distribution flip(0.5);
    std::vector<DrawnCard> drawn;
    for (std::size_t i = 0; i < n && i < order.size(); ++i) drawn.push_back(DrawnCard{deck_[order[i]], flip(rng())});
    return drawn;
}

DrawnCard Tarot::cardOfTheDay(const std::string& person, const Date& day) const {
    const unsigned long long h = stableHash(lower(person) + "|" + day.iso());
    return DrawnCard{deck_[static_cast<std::size_t>(h % deck_.size())], ((h >> 32) & 1ULL) == 1ULL};
}

std::vector<TarotCard> Tarot::search(const std::string& text) const {
    const std::string needle = lower(text);
    std::vector<TarotCard> hits;
    for (const TarotCard& c : deck_)
        if (lower(c.name).find(needle) != std::string::npos) hits.push_back(c);
    return hits;
}

std::vector<TarotCard> Tarot::group(const std::string& arcanaOrSuit) const {
    const std::string key = lower(arcanaOrSuit);
    std::vector<TarotCard> hits;
    for (const TarotCard& c : deck_)
        if (c.arcana == key || c.suit == key) hits.push_back(c);
    std::sort(hits.begin(), hits.end(), [](const TarotCard& a, const TarotCard& b) { return a.value < b.value; });
    return hits;
}
