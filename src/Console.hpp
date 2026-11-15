#pragma once
// Terminal helpers. Colors and screen clearing switch off automatically when
// input/output is piped (tests, scripts) or NO_COLOR is set.
#include <optional>
#include <string>

namespace console {

void setup();                      // UTF-8 + ANSI colors on Windows
bool interactive();                // stdin and stdout are a real terminal

std::string bold(const std::string& s);
std::string dim(const std::string& s);
std::string accent(const std::string& s);   // magenta
std::string good(const std::string& s);     // green
std::string warn(const std::string& s);     // yellow

void clear();
void rule();                                // a horizontal line
void wrap(const std::string& text, const std::string& indent = "  ", std::size_t width = 76);

// Prompts. Return nullopt when input has ended (Ctrl+Z / Ctrl+D or end of a pipe).
std::optional<std::string> ask(const std::string& prompt);
std::optional<int> askNumber(const std::string& prompt);
void pause();                               // "Press Enter to continue..." (skipped when piped)

}  // namespace console
