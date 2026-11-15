#pragma once
// Just enough JSON for the horoscope API: read a string field by name.
#include <optional>
#include <string>

// Finds the first `"key": "value"` pair anywhere in `json` and returns the
// decoded value (handles \" \\ \/ \n \t \uXXXX, including surrogate pairs).
// Returns nullopt if the key is missing or its value isn't a string.
std::optional<std::string> jsonString(const std::string& json, const std::string& key);
