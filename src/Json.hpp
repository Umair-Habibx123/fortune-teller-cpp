#pragma once
// A small JSON parser: enough for the API's nested objects, arrays and numbers.
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    static std::optional<Json> parse(const std::string& text);

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isObject() const { return type_ == Type::Object; }
    bool isArray() const { return type_ == Type::Array; }

    // Object member or array item; returns a shared null value when missing.
    const Json& operator[](const std::string& key) const;
    const Json& operator[](std::size_t index) const;
    bool has(const std::string& key) const;

    std::size_t size() const;  // array length or number of object members
    const std::vector<std::pair<std::string, Json>>& members() const { return object_; }
    const std::vector<Json>& items() const { return array_; }

    // Value or fallback when the type doesn't match.
    std::string str(const std::string& fallback = "") const;
    double num(double fallback = 0.0) const;
    bool boolean(bool fallback = false) const;

private:
    friend class JsonParser;
    Type type_ = Type::Null;
    bool bool_ = false;
    double number_ = 0.0;
    std::string string_;
    std::vector<Json> array_;
    std::vector<std::pair<std::string, Json>> object_;
};

// Shortcut used by the horoscope code: the first string field with this name,
// searched at any depth. nullopt if the JSON is invalid or the field is missing.
std::optional<std::string> jsonString(const std::string& json, const std::string& key);
