#pragma once
// Tarot from FreeHoroscopeAPI's 78-card deck. The deck is downloaded once and
// kept, so draws work offline afterwards.
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Date.hpp"
#include "Json.hpp"

struct TarotCard {
    std::string name;        // "The Fool", "Three of Cups"
    std::string shortName;   // "ar00", "cu03"
    std::string arcana;      // "major" / "minor"
    std::string suit;        // wands, cups, swords, pentacles (minor only)
    int value = 0;
    std::string meaningUp;
    std::string meaningReversed;
    std::string description;
};

struct DrawnCard {
    TarotCard card;
    bool reversed = false;
    const std::string& meaning() const { return reversed ? card.meaningReversed : card.meaningUp; }
};

class Tarot {
public:
    explicit Tarot(std::filesystem::path cacheDir);

    // Loads the deck (from disk, or downloads it the first time). Returns a
    // reason if it isn't available.
    std::string load(const Date& today);
    bool ready() const { return !deck_.empty(); }
    const std::vector<TarotCard>& deck() const { return deck_; }

    // `n` different cards, each upright or reversed at random.
    std::vector<DrawnCard> draw(std::size_t n) const;
    // One card that stays the same all day for this person.
    DrawnCard cardOfTheDay(const std::string& person, const Date& day) const;

    std::vector<TarotCard> search(const std::string& text) const;    // by name, case-insensitive
    std::vector<TarotCard> group(const std::string& arcanaOrSuit) const;  // "major", "cups", ...

    static std::vector<TarotCard> parseDeck(const Json& doc);

private:
    std::filesystem::path dir_;
    std::vector<TarotCard> deck_;
};
