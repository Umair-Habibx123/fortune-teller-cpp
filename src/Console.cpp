#include "Console.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace console {
namespace {

bool g_color = false;
bool g_interactive = false;

std::string paint(const char* code, const std::string& s) { return g_color ? std::string("\x1b[") + code + "m" + s + "\x1b[0m" : s; }

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
    std::size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t')) ++start;
    return s.substr(start);
}

}  // namespace

void setup() {
    g_interactive = ISATTY(FILENO(stdin)) && ISATTY(FILENO(stdout));
    bool colorOk = g_interactive && !std::getenv("NO_COLOR");
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (colorOk) {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        colorOk = GetConsoleMode(out, &mode) && SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
    g_color = colorOk;
}

bool interactive() { return g_interactive; }

std::string bold(const std::string& s) { return paint("1", s); }
std::string dim(const std::string& s) { return paint("2", s); }
std::string accent(const std::string& s) { return paint("1;35", s); }
std::string good(const std::string& s) { return paint("32", s); }
std::string warn(const std::string& s) { return paint("33", s); }

void clear() {
    if (g_interactive) std::cout << "\x1b[2J\x1b[H" << std::flush;
    else std::cout << '\n';
}

void rule() { std::cout << "  " << dim(std::string(70, '-')) << '\n'; }

void wrap(const std::string& text, const std::string& indent, std::size_t width) {
    std::istringstream paragraphs(text);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string word, line;
        while (words >> word) {
            if (!line.empty() && line.size() + 1 + word.size() > width) {
                std::cout << indent << line << '\n';
                line.clear();
            }
            line += (line.empty() ? "" : " ") + word;
        }
        std::cout << indent << line << '\n';
    }
}

std::optional<std::string> ask(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << '\n';
        return std::nullopt;
    }
    if (!g_interactive) std::cout << line << '\n';  // echo piped input so transcripts read naturally
    return trim(line);
}

std::optional<int> askNumber(const std::string& prompt) {
    for (;;) {
        auto line = ask(prompt);
        if (!line) return std::nullopt;
        try {
            std::size_t used = 0;
            const int n = std::stoi(*line, &used);
            if (used == line->size()) return n;
        } catch (...) {
        }
        std::cout << warn("  Please type a number.") << '\n';
    }
}

void pause() {
    if (!g_interactive) return;
    std::cout << dim("\n  Press Enter to continue...") << std::flush;
    std::string ignored;
    std::getline(std::cin, ignored);
}

}  // namespace console
