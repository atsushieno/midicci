#include <gtest/gtest.h>
#include "midi-ci/json/Json.hpp"

using namespace midi_ci::json;

TEST(JsonCompleteTest, parseString) {
    std::string input = "\"hello\"";
    auto result = JsonValue::parse(input);
    EXPECT_EQ("hello", result.as_string());
}

TEST(JsonCompleteTest, parseNull) {
    std::string input = "null";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.is_null());
}

TEST(JsonCompleteTest, parseTrue) {
    std::string input = "true";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.as_bool());
}

TEST(JsonCompleteTest, parseFalse) {
    std::string input = "false";
    auto result = JsonValue::parse(input);
    EXPECT_FALSE(result.as_bool());
}

TEST(JsonCompleteTest, parseNumber) {
    std::string input = "42";
    auto result = JsonValue::parse(input);
    EXPECT_EQ(42, result.as_int());
}

TEST(JsonCompleteTest, parseFloatNumber) {
    std::string input = "3.14";
    auto result = JsonValue::parse(input);
    EXPECT_DOUBLE_EQ(3.14, result.as_number());
}

TEST(JsonCompleteTest, parseNegativeNumber) {
    std::string input = "-123";
    auto result = JsonValue::parse(input);
    EXPECT_EQ(-123, result.as_int());
}

TEST(JsonCompleteTest, parseEmptyObject) {
    std::string input = "{}";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(0, result.as_object().size());
}

TEST(JsonCompleteTest, parseSimpleObject) {
    std::string input = "{\"key\": \"value\"}";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.is_object());
    auto obj = result.as_object();
    EXPECT_EQ(1, obj.size());
    EXPECT_EQ("value", obj.at("key").as_string());
}

TEST(JsonCompleteTest, parseEmptyArray) {
    std::string input = "[]";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(0, result.as_array().size());
}

TEST(JsonCompleteTest, parseSimpleArray) {
    std::string input = "[1, 2, 3]";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.is_array());
    auto arr = result.as_array();
    EXPECT_EQ(3, arr.size());
    EXPECT_EQ(1, arr[0].as_int());
    EXPECT_EQ(2, arr[1].as_int());
    EXPECT_EQ(3, arr[2].as_int());
}

TEST(JsonCompleteTest, parseNestedObject) {
    std::string input = "{\"outer\": {\"inner\": \"value\"}}";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.is_object());
    auto obj = result.as_object();
    EXPECT_EQ(1, obj.size());
    auto& outer = obj.at("outer");
    EXPECT_TRUE(outer.is_object());
    EXPECT_EQ("value", outer.as_object().at("inner").as_string());
}

TEST(JsonCompleteTest, parseComplexStructure) {
    std::string input = "{\"numbers\": [1, 2, 3], \"boolean\": true, \"null_value\": null}";
    auto result = JsonValue::parse(input);
    EXPECT_TRUE(result.is_object());
    auto obj = result.as_object();
    EXPECT_EQ(3, obj.size());
    
    auto& numbers = obj.at("numbers");
    EXPECT_TRUE(numbers.is_array());
    EXPECT_EQ(3, numbers.as_array().size());
    
    auto& boolean = obj.at("boolean");
    EXPECT_TRUE(boolean.as_bool());
    
    auto& nullValue = obj.at("null_value");
    EXPECT_TRUE(nullValue.is_null());
}
