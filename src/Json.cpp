#include "Json.hpp"

#include <cctype>

namespace {

void appendUtf8(std::string& out, unsigned long cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

bool hex4(const std::string& s, std::size_t at, unsigned long& out) {
    if (at + 4 > s.size()) return false;
    out = 0;
    for (std::size_t i = at; i < at + 4; ++i) {
        const char c = s[i];
        out <<= 4;
        if (c >= '0' && c <= '9') out |= static_cast<unsigned long>(c - '0');
        else if (c >= 'a' && c <= 'f') out |= static_cast<unsigned long>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') out |= static_cast<unsigned long>(c - 'A' + 10);
        else return false;
    }
    return true;
}

// Reads a JSON string starting at s[pos] == '"'. Advances pos past the closing quote.
std::optional<std::string> readString(const std::string& s, std::size_t& pos) {
    if (pos >= s.size() || s[pos] != '"') return std::nullopt;
    std::string out;
    for (++pos; pos < s.size(); ++pos) {
        const char c = s[pos];
        if (c == '"') {
            ++pos;
            return out;
        }
        if (c != '\\') {
            out += c;
            continue;
        }
        if (++pos >= s.size()) return std::nullopt;
        switch (s[pos]) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                unsigned long cp = 0;
                if (!hex4(s, pos + 1, cp)) return std::nullopt;
                pos += 4;
                if (cp >= 0xD800 && cp <= 0xDBFF && pos + 6 < s.size() && s[pos + 1] == '\\' && s[pos + 2] == 'u') {
                    unsigned long low = 0;
                    if (hex4(s, pos + 3, low) && low >= 0xDC00 && low <= 0xDFFF) {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        pos += 6;
                    }
                }
                appendUtf8(out, cp);
                break;
            }
            default: return std::nullopt;
        }
    }
    return std::nullopt;  // unterminated
}

}  // namespace

std::optional<std::string> jsonString(const std::string& json, const std::string& key) {
    std::size_t pos = 0;
    while (pos < json.size()) {
        if (json[pos] != '"') {
            ++pos;
            continue;
        }
        auto token = readString(json, pos);  // a key or a string value
        if (!token) return std::nullopt;
        std::size_t after = pos;
        while (after < json.size() && std::isspace(static_cast<unsigned char>(json[after]))) ++after;
        if (after < json.size() && json[after] == ':' && *token == key) {
            ++after;
            while (after < json.size() && std::isspace(static_cast<unsigned char>(json[after]))) ++after;
            if (after < json.size() && json[after] == '"') return readString(json, after);
            return std::nullopt;
        }
    }
    return std::nullopt;
}
