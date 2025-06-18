#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <cstdint>

namespace midicci {
namespace json_ish {

class JsonValue;

using JsonNull = std::nullptr_t;
using JsonBool = bool;
using JsonNumber = double;
using JsonString = std::string;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

class JsonValue {
public:
    using ValueType = std::variant<JsonNull, JsonBool, JsonNumber, JsonString, JsonArray, JsonObject>;
    
    JsonValue() : value_(nullptr) {}
    JsonValue(JsonNull) : value_(nullptr) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(int i) : value_(static_cast<double>(i)) {}
    JsonValue(double d) : value_(d) {}
    JsonValue(const char* s) : value_(std::string(s)) {}
    JsonValue(const std::string& s) : value_(s) {}
    JsonValue(std::string&& s) : value_(std::move(s)) {}
    JsonValue(const JsonArray& arr) : value_(arr) {}
    JsonValue(JsonArray&& arr) : value_(std::move(arr)) {}
    JsonValue(const JsonObject& obj) : value_(obj) {}
    JsonValue(JsonObject&& obj) : value_(std::move(obj)) {}
    
    bool is_null() const { return std::holds_alternative<JsonNull>(value_); }
    bool is_bool() const { return std::holds_alternative<JsonBool>(value_); }
    bool is_number() const { return std::holds_alternative<JsonNumber>(value_); }
    bool is_string() const { return std::holds_alternative<JsonString>(value_); }
    bool is_array() const { return std::holds_alternative<JsonArray>(value_); }
    bool is_object() const { return std::holds_alternative<JsonObject>(value_); }
    
    bool as_bool() const { return is_bool() ? std::get<JsonBool>(value_) : false; }
    double as_number() const { return is_number() ? std::get<JsonNumber>(value_) : 0.0; }
    int as_int() const { return static_cast<int>(as_number()); }
    const std::string& as_string() const { 
        static const std::string empty;
        return is_string() ? std::get<JsonString>(value_) : empty; 
    }
    const JsonArray& as_array() const { 
        static const JsonArray empty;
        return is_array() ? std::get<JsonArray>(value_) : empty; 
    }
    const JsonObject& as_object() const { 
        static const JsonObject empty;
        return is_object() ? std::get<JsonObject>(value_) : empty; 
    }
    
    JsonValue& operator[](const std::string& key);
    const JsonValue& operator[](const std::string& key) const;
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](size_t index) const;
    
    std::vector<uint8_t> get_serialized_bytes() const;
    
    static JsonValue parse(const std::string& json_str);
    static JsonValue parse_or_null(const std::string& json_str);
    
    std::string serialize() const;
    
    static const JsonValue& null_value();
    static const JsonValue& true_value();
    static const JsonValue& false_value();
    static JsonValue empty_object();
    static JsonValue empty_array();
    
private:
    ValueType value_;
};

class JsonParser {
public:
    static JsonValue parse(const std::string& json_str);
    
private:
    JsonParser(const std::string& json_str);
    JsonValue parse_value();
    JsonValue parse_object();
    JsonValue parse_array();
    JsonValue parse_string();
    JsonValue parse_number();
    JsonValue parse_literal();
    
    void skip_whitespace();
    char peek() const;
    char next();
    bool has_more() const;
    
    const std::string& json_;
    size_t pos_;
};

std::string escape_string(const std::string& str);
std::string unescape_string(const std::string& str);

} // namespace json
} // namespace midi_ci
