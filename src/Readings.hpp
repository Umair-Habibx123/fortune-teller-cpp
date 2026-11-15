#pragma once
// The offline readings from the original program, as plain functions.
#include <string>
#include <vector>

#include "Zodiac.hpp"

// "About you": the original picked notes by birthday. With step = the last digit
// of the day (5 when it's 0), it showed notes step, 2*step, ... up to 5 of them.
std::vector<std::string> aboutYou(Sign sign, int birthDay);

// The original's four lucky numbers, from the day of birth.
std::vector<int> luckyNumbers(int birthDay);

// A random fortune. FORTUNE_SEED makes it repeatable (used by the tests).
std::string fortuneCookie();

// The love-compatibility text for two signs.
const std::string& compatibility(Sign a, Sign b);

// Chinese zodiac animal by birth year. People born in January or early February
// may belong to the previous year's animal (the Chinese year starts then).
std::string chineseAnimal(int year);

// The classic element pairing: same element, a natural match (Fire+Air,
// Earth+Water), or a harder mix.
std::string elementHarmony(Sign a, Sign b);
