#include "Json.hpp"

#include <cctype>
#include <cstdlib>

namespace {

const Json& nullValue() {
    static const Json value;
    return value;
}

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

}  // namespace

class JsonParser {
public:
    explicit JsonParser(const std::string& s) : s_(s) {}

    std::optional<Json> document() {
        Json value;
        if (!parseValue(value, 0)) return std::nullopt;
        skipSpace();
        if (pos_ != s_.size()) return std::nullopt;
        return value;
    }

private:
    static constexpr int kMaxDepth = 64;

    void skipSpace() {
        while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_;
    }

    bool literal(const char* word) {
        std::size_t i = 0;
        while (word[i]) {
            if (pos_ + i >= s_.size() || s_[pos_ + i] != word[i]) return false;
            ++i;
        }
        pos_ += i;
        return true;
    }

    bool hex4(unsigned long& out) {
        if (pos_ + 4 > s_.size()) return false;
        out = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = s_[pos_++];
            out <<= 4;
            if (c >= '0' && c <= '9') out |= static_cast<unsigned long>(c - '0');
            else if (c >= 'a' && c <= 'f') out |= static_cast<unsigned long>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') out |= static_cast<unsigned long>(c - 'A' + 10);
            else return false;
        }
        return true;
    }

    bool parseString(std::string& out) {
        if (pos_ >= s_.size() || s_[pos_] != '"') return false;
        ++pos_;
        while (pos_ < s_.size()) {
            const char c = s_[pos_++];
            if (c == '"') return true;
            if (static_cast<unsigned char>(c) < 0x20) return false;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos_ >= s_.size()) return false;
            switch (s_[pos_++]) {
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
                    if (!hex4(cp)) return false;
                    if (cp >= 0xD800 && cp <= 0xDBFF && pos_ + 1 < s_.size() && s_[pos_] == '\\' && s_[pos_ + 1] == 'u') {
                        const std::size_t save = pos_;
                        pos_ += 2;
                        unsigned long low = 0;
                        if (hex4(low) && low >= 0xDC00 && low <= 0xDFFF) cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        else pos_ = save;
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default: return false;
            }
        }
        return false;
    }

    bool parseNumber(Json& out) {
        const std::size_t start = pos_;
        if (pos_ < s_.size() && (s_[pos_] == '-' || s_[pos_] == '+')) ++pos_;
        while (pos_ < s_.size() && (std::isdigit(static_cast<unsigned char>(s_[pos_])) || s_[pos_] == '.' || s_[pos_] == 'e' ||
                                    s_[pos_] == 'E' || s_[pos_] == '-' || s_[pos_] == '+'))
            ++pos_;
        if (pos_ == start) return false;
        const std::string text = s_.substr(start, pos_ - start);
        char* end = nullptr;
        out.number_ = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size()) return false;
        out.type_ = Json::Type::Number;
        return true;
    }

    bool parseValue(Json& out, int depth) {
        if (depth > kMaxDepth) return false;
        skipSpace();
        if (pos_ >= s_.size()) return false;
        const char c = s_[pos_];
        if (c == '{') {
            ++pos_;
            out.type_ = Json::Type::Object;
            skipSpace();
            if (pos_ < s_.size() && s_[pos_] == '}') {
                ++pos_;
                return true;
            }
            for (;;) {
                skipSpace();
                std::string key;
                if (!parseString(key)) return false;
                skipSpace();
                if (pos_ >= s_.size() || s_[pos_] != ':') return false;
                ++pos_;
                Json value;
                if (!parseValue(value, depth + 1)) return false;
                out.object_.emplace_back(std::move(key), std::move(value));
                skipSpace();
                if (pos_ < s_.size() && s_[pos_] == ',') {
                    ++pos_;
                    continue;
                }
                if (pos_ < s_.size() && s_[pos_] == '}') {
                    ++pos_;
                    return true;
                }
                return false;
            }
        }
        if (c == '[') {
            ++pos_;
            out.type_ = Json::Type::Array;
            skipSpace();
            if (pos_ < s_.size() && s_[pos_] == ']') {
                ++pos_;
                return true;
            }
            for (;;) {
                Json value;
                if (!parseValue(value, depth + 1)) return false;
                out.array_.push_back(std::move(value));
                skipSpace();
                if (pos_ < s_.size() && s_[pos_] == ',') {
                    ++pos_;
                    continue;
                }
                if (pos_ < s_.size() && s_[pos_] == ']') {
                    ++pos_;
                    return true;
                }
                return false;
            }
        }
        if (c == '"') {
            out.type_ = Json::Type::String;
            return parseString(out.string_);
        }
        if (literal("true")) {
            out.type_ = Json::Type::Bool;
            out.bool_ = true;
            return true;
        }
        if (literal("false")) {
            out.type_ = Json::Type::Bool;
            return true;
        }
        if (literal("null")) return true;
        return parseNumber(out);
    }

    const std::string& s_;
    std::size_t pos_ = 0;
};

std::optional<Json> Json::parse(const std::string& text) { return JsonParser(text).document(); }

const Json& Json::operator[](const std::string& key) const {
    for (const auto& member : object_)
        if (member.first == key) return member.second;
    return nullValue();
}

const Json& Json::operator[](std::size_t index) const { return index < array_.size() ? array_[index] : nullValue(); }

bool Json::has(const std::string& key) const {
    for (const auto& member : object_)
        if (member.first == key) return true;
    return false;
}

std::size_t Json::size() const { return type_ == Type::Array ? array_.size() : object_.size(); }
std::string Json::str(const std::string& fallback) const { return type_ == Type::String ? string_ : fallback; }
double Json::num(double fallback) const { return type_ == Type::Number ? number_ : fallback; }
bool Json::boolean(bool fallback) const { return type_ == Type::Bool ? bool_ : fallback; }

namespace {

const Json* findString(const Json& node, const std::string& key) {
    for (const auto& member : node.members()) {
        if (member.first == key && member.second.type() == Json::Type::String) return &member.second;
    }
    for (const auto& member : node.members())
        if (const Json* hit = findString(member.second, key)) return hit;
    for (const auto& item : node.items())
        if (const Json* hit = findString(item, key)) return hit;
    return nullptr;
}

}  // namespace

std::optional<std::string> jsonString(const std::string& json, const std::string& key) {
    auto doc = Json::parse(json);
    if (!doc) return std::nullopt;
    if (const Json* hit = findString(*doc, key)) return hit->str();
    return std::nullopt;
}
