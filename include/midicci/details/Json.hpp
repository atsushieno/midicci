#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <cstdint>

namespace midicci {

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
    
    bool isNull() const { return std::holds_alternative<JsonNull>(value_); }
    bool isBool() const { return std::holds_alternative<JsonBool>(value_); }
    bool isNumber() const { return std::holds_alternative<JsonNumber>(value_); }
    bool isString() const { return std::holds_alternative<JsonString>(value_); }
    bool isArray() const { return std::holds_alternative<JsonArray>(value_); }
    bool isObject() const { return std::holds_alternative<JsonObject>(value_); }
    
    bool asBool() const { return isBool() ? std::get<JsonBool>(value_) : false; }
    double asNumber() const { return isNumber() ? std::get<JsonNumber>(value_) : 0.0; }
    int asInt() const { return static_cast<int>(asNumber()); }
    const std::string& asString() const { 
        static const std::string empty;
        return isString() ? std::get<JsonString>(value_) : empty; 
    }
    const JsonArray& asArray() const { 
        static const JsonArray empty;
        return isArray() ? std::get<JsonArray>(value_) : empty; 
    }
    const JsonObject& asObject() const { 
        static const JsonObject empty;
        return isObject() ? std::get<JsonObject>(value_) : empty; 
    }
    
    JsonValue& operator[](const std::string& key);
    const JsonValue& operator[](const std::string& key) const;
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](size_t index) const;
    
    std::vector<uint8_t> getSerializedBytes() const;
    
    static JsonValue parse(const std::string& json_str);
    static JsonValue parseOrNull(const std::string& json_str);
    
    std::string serialize() const;
    
    static const JsonValue& null_value();
    static const JsonValue& trueValue();
    static const JsonValue& falseValue();
    static JsonValue emptyObject();
    static JsonValue emptyArray();
    
private:
    ValueType value_;
};

class JsonParser {
public:
    static JsonValue parse(const std::string& json_str);
    
private:
    JsonParser(const std::string& json_str);
    JsonValue parseValue();
    JsonValue parseObject();
    JsonValue parseArray();
    JsonValue parseString();
    JsonValue parseNumber();
    JsonValue parseLiteral();
    
    void skipWhitespace();
    char peek() const;
    char next();
    bool hasMore() const;
    
    const std::string& json_;
    size_t pos_;
};

std::string escapeString(const std::string& str);
std::string unescapeString(const std::string& str);

} // namespace midi_ci
