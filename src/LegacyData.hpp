#pragma once
// Texts from the original first-semester version (see tools/extract_legacy.py).
#include <string>
#include <vector>

namespace legacy {

// Personality notes per sign, in Sign order (Aries .. Pisces).
extern const std::vector<std::vector<std::string>> kAbout;

// 78 love-compatibility texts: one for every unordered pair of signs,
// ordered Aries-Aries, Aries-Taurus, ... Aries-Pisces, Taurus-Taurus, ...
extern const std::vector<std::string> kLove;

// Fortune-cookie lines.
extern const std::vector<std::string> kFortunes;

// ASCII drawing of each sign, in Sign order.
extern const std::vector<std::vector<std::string>> kArt;

} // namespace legacy
