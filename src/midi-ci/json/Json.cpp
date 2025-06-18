#include "midi-ci/json/Json.hpp"
#include "midi-ci/core/MidiCIConverter.hpp"
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <iomanip>

namespace midi_ci {
namespace json {

JsonValue& JsonValue::operator[](const std::string& key) {
    if (!is_object()) {
        value_ = JsonObject{};
    }
    return std::get<JsonObject>(value_)[key];
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (is_object()) {
        const auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        if (it != obj.end()) {
            return it->second;
        }
    }
    return null_value();
}

JsonValue& JsonValue::operator[](size_t index) {
    if (!is_array()) {
        value_ = JsonArray{};
    }
    auto& arr = std::get<JsonArray>(value_);
    if (index >= arr.size()) {
        arr.resize(index + 1);
    }
    return arr[index];
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (is_array()) {
        const auto& arr = std::get<JsonArray>(value_);
        if (index < arr.size()) {
            return arr[index];
        }
    }
    return null_value();
}

std::vector<uint8_t> JsonValue::get_serialized_bytes() const {
    auto json_str = serialize();
    auto ascii_encoded = midi_ci::core::MidiCIConverter::encodeStringToASCII(json_str);
    auto escaped = ascii_encoded;
    size_t pos = 0;
    while ((pos = escaped.find("\\", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\\");
        pos += 2;
    }
    std::vector<uint8_t> result;
    result.reserve(escaped.size());
    for (char c : escaped) {
        result.push_back(static_cast<uint8_t>(c));
    }
    return result;
}

JsonValue JsonValue::parse(const std::string& json_str) {
    return JsonParser::parse(json_str);
}

JsonValue JsonValue::parse_or_null(const std::string& json_str) {
    try {
        return parse(json_str);
    } catch (...) {
        return JsonValue{};
    }
}

std::string JsonValue::serialize() const {
    std::ostringstream oss;
    
    std::visit([&oss](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, JsonNull>) {
            oss << "null";
        } else if constexpr (std::is_same_v<T, JsonBool>) {
            oss << (value ? "true" : "false");
        } else if constexpr (std::is_same_v<T, JsonNumber>) {
            if (value == static_cast<int>(value)) {
                oss << static_cast<int>(value);
            } else {
                oss << value;
            }
        } else if constexpr (std::is_same_v<T, JsonString>) {
            oss << '"' << escape_string(value) << '"';
        } else if constexpr (std::is_same_v<T, JsonArray>) {
            oss << '[';
            for (size_t i = 0; i < value.size(); ++i) {
                if (i > 0) oss << ',';
                oss << value[i].serialize();
            }
            oss << ']';
        } else if constexpr (std::is_same_v<T, JsonObject>) {
            oss << '{';
            bool first = true;
            for (const auto& [key, val] : value) {
                if (!first) oss << ',';
                first = false;
                oss << '"' << escape_string(key) << '"' << ':' << val.serialize();
            }
            oss << '}';
        }
    }, value_);
    
    return oss.str();
}

const JsonValue& JsonValue::null_value() {
    static const JsonValue null_val{};
    return null_val;
}

const JsonValue& JsonValue::true_value() {
    static const JsonValue true_val{true};
    return true_val;
}

const JsonValue& JsonValue::false_value() {
    static const JsonValue false_val{false};
    return false_val;
}

JsonValue JsonValue::empty_object() {
    return JsonValue{JsonObject{}};
}

JsonValue JsonValue::empty_array() {
    return JsonValue{JsonArray{}};
}

JsonParser::JsonParser(const std::string& json_str) : json_(json_str), pos_(0) {}

JsonValue JsonParser::parse(const std::string& json_str) {
    JsonParser parser(json_str);
    return parser.parse_value();
}

JsonValue JsonParser::parse_value() {
    skip_whitespace();
    
    if (!has_more()) {
        throw std::runtime_error("Unexpected end of JSON input");
    }
    
    char c = peek();
    switch (c) {
        case '{': return parse_object();
        case '[': return parse_array();
        case '"': return parse_string();
        case 't': case 'f': case 'n': return parse_literal();
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number();
        default:
            throw std::runtime_error("Unexpected character in JSON");
    }
}

JsonValue JsonParser::parse_object() {
    JsonObject obj;
    next(); // consume '{'
    skip_whitespace();
    
    if (peek() == '}') {
        next(); // consume '}'
        return JsonValue{std::move(obj)};
    }
    
    while (has_more()) {
        skip_whitespace();
        
        if (peek() != '"') {
            throw std::runtime_error("Expected string key in JSON object");
        }
        
        auto key = parse_string().as_string();
        skip_whitespace();
        
        if (next() != ':') {
            throw std::runtime_error("Expected ':' after key in JSON object");
        }
        
        auto value = parse_value();
        obj[key] = std::move(value);
        
        skip_whitespace();
        char c = next();
        if (c == '}') {
            break;
        } else if (c != ',') {
            throw std::runtime_error("Expected ',' or '}' in JSON object");
        }
    }
    
    return JsonValue{std::move(obj)};
}

JsonValue JsonParser::parse_array() {
    JsonArray arr;
    next(); // consume '['
    skip_whitespace();
    
    if (peek() == ']') {
        next(); // consume ']'
        return JsonValue{std::move(arr)};
    }
    
    while (has_more()) {
        arr.push_back(parse_value());
        skip_whitespace();
        
        char c = next();
        if (c == ']') {
            break;
        } else if (c != ',') {
            throw std::runtime_error("Expected ',' or ']' in JSON array");
        }
    }
    
    return JsonValue{std::move(arr)};
}

JsonValue JsonParser::parse_string() {
    next(); // consume '"'
    std::string str;
    
    while (has_more()) {
        char c = next();
        if (c == '"') {
            return JsonValue{unescape_string(str)};
        } else if (c == '\\') {
            if (!has_more()) {
                throw std::runtime_error("Unexpected end of string");
            }
            char escaped = next();
            switch (escaped) {
                case '"': str += '"'; break;
                case '\\': str += '\\'; break;
                case '/': str += '/'; break;
                case 'b': str += '\b'; break;
                case 'f': str += '\f'; break;
                case 'n': str += '\n'; break;
                case 'r': str += '\r'; break;
                case 't': str += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > json_.size()) {
                        throw std::runtime_error("Invalid unicode escape");
                    }
                    std::string hex = json_.substr(pos_, 4);
                    pos_ += 4;
                    int codepoint = std::stoi(hex, nullptr, 16);
                    if (codepoint <= 0x7F) {
                        str += static_cast<char>(codepoint);
                    } else if (codepoint <= 0x7FF) {
                        str += static_cast<char>(0xC0 | (codepoint >> 6));
                        str += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else {
                        str += static_cast<char>(0xE0 | (codepoint >> 12));
                        str += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        str += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Invalid escape sequence");
            }
        } else {
            str += c;
        }
    }
    
    throw std::runtime_error("Unterminated string");
}

JsonValue JsonParser::parse_number() {
    size_t start = pos_;
    
    if (peek() == '-') {
        next();
    }
    
    if (!std::isdigit(peek())) {
        throw std::runtime_error("Invalid number format");
    }
    
    while (has_more() && std::isdigit(peek())) {
        next();
    }
    
    if (has_more() && peek() == '.') {
        next();
        if (!has_more() || !std::isdigit(peek())) {
            throw std::runtime_error("Invalid number format");
        }
        while (has_more() && std::isdigit(peek())) {
            next();
        }
    }
    
    if (has_more() && (peek() == 'e' || peek() == 'E')) {
        next();
        if (has_more() && (peek() == '+' || peek() == '-')) {
            next();
        }
        if (!has_more() || !std::isdigit(peek())) {
            throw std::runtime_error("Invalid number format");
        }
        while (has_more() && std::isdigit(peek())) {
            next();
        }
    }
    
    std::string num_str = json_.substr(start, pos_ - start);
    return JsonValue{std::stod(num_str)};
}

JsonValue JsonParser::parse_literal() {
    if (json_.substr(pos_, 4) == "null") {
        pos_ += 4;
        return JsonValue{};
    } else if (json_.substr(pos_, 4) == "true") {
        pos_ += 4;
        return JsonValue{true};
    } else if (json_.substr(pos_, 5) == "false") {
        pos_ += 5;
        return JsonValue{false};
    } else {
        throw std::runtime_error("Invalid literal");
    }
}

void JsonParser::skip_whitespace() {
    while (has_more() && std::isspace(peek())) {
        next();
    }
}

char JsonParser::peek() const {
    return has_more() ? json_[pos_] : '\0';
}

char JsonParser::next() {
    return has_more() ? json_[pos_++] : '\0';
}

bool JsonParser::has_more() const {
    return pos_ < json_.size();
}

std::string escape_string(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

std::string unescape_string(const std::string& str) {
    if (str.find('\\') == std::string::npos) {
        return str;
    }
    
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            char next = str[i + 1];
            switch (next) {
                case '"':
                    result += '"';
                    i++;
                    break;
                case '\\':
                    result += '\\';
                    i++;
                    break;
                case '/':
                    result += '/';
                    i++;
                    break;
                case 'b':
                    result += '\b';
                    i++;
                    break;
                case 'f':
                    result += '\f';
                    i++;
                    break;
                case 'n':
                    result += '\n';
                    i++;
                    break;
                case 'r':
                    result += '\r';
                    i++;
                    break;
                case 't':
                    result += '\t';
                    i++;
                    break;
                case 'u':
                    if (i + 5 < str.size()) {
                        std::string hex = str.substr(i + 2, 4);
                        try {
                            int codepoint = std::stoi(hex, nullptr, 16);
                            if (codepoint <= 0x7F) {
                                result += static_cast<char>(codepoint);
                            } else if (codepoint <= 0x7FF) {
                                result += static_cast<char>(0xC0 | (codepoint >> 6));
                                result += static_cast<char>(0x80 | (codepoint & 0x3F));
                            } else {
                                result += static_cast<char>(0xE0 | (codepoint >> 12));
                                result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (codepoint & 0x3F));
                            }
                            i += 5;
                        } catch (...) {
                            result += str[i];
                        }
                    } else {
                        result += str[i];
                    }
                    break;
                default:
                    result += str[i];
                    break;
            }
        } else {
            result += str[i];
        }
    }
    
    return result;
}

} // namespace json
} // namespace midi_ci
