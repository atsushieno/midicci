#include <gtest/gtest.h>
#include "midi-ci/json/Json.hpp"

using namespace midi_ci::json;

class JsonSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(JsonSerializationTest, EmptyObject) {
    JsonValue obj = JsonValue::empty_object();
    EXPECT_TRUE(obj.is_object());
    EXPECT_EQ(obj.to_string(), "{}");
}

TEST_F(JsonSerializationTest, SimpleObject) {
    JsonValue obj = JsonValue::empty_object();
    obj["resource"] = JsonValue("DeviceInfo");
    obj["resId"] = JsonValue("device1");
    
    std::string json_str = obj.to_string();
    EXPECT_TRUE(json_str.find("\"resource\":\"DeviceInfo\"") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"resId\":\"device1\"") != std::string::npos);
}

TEST_F(JsonSerializationTest, BooleanValues) {
    JsonValue obj = JsonValue::empty_object();
    obj["setPartial"] = JsonValue(true);
    obj["enabled"] = JsonValue(false);
    
    std::string json_str = obj.to_string();
    EXPECT_TRUE(json_str.find("\"setPartial\":true") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"enabled\":false") != std::string::npos);
}

TEST_F(JsonSerializationTest, NumericValues) {
    JsonValue obj = JsonValue::empty_object();
    obj["offset"] = JsonValue(42);
    obj["limit"] = JsonValue(100);
    
    std::string json_str = obj.to_string();
    EXPECT_TRUE(json_str.find("\"offset\":42") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"limit\":100") != std::string::npos);
}

TEST_F(JsonSerializationTest, SerializedBytes) {
    JsonValue obj = JsonValue::empty_object();
    obj["resource"] = JsonValue("TestResource");
    obj["value"] = JsonValue(123);
    
    auto bytes = obj.get_serialized_bytes();
    EXPECT_FALSE(bytes.empty());
    
    std::string json_str(bytes.begin(), bytes.end());
    EXPECT_TRUE(json_str.find("TestResource") != std::string::npos);
    EXPECT_TRUE(json_str.find("123") != std::string::npos);
}

TEST_F(JsonSerializationTest, ParseSimpleObject) {
    std::string json_str = "{\"resource\":\"DeviceInfo\",\"resId\":\"device1\"}";
    JsonValue parsed = JsonValue::parse(json_str);
    
    EXPECT_TRUE(parsed.is_object());
    EXPECT_EQ(parsed["resource"].as_string(), "DeviceInfo");
    EXPECT_EQ(parsed["resId"].as_string(), "device1");
}

TEST_F(JsonSerializationTest, ParseWithBooleans) {
    std::string json_str = "{\"setPartial\":true,\"enabled\":false}";
    JsonValue parsed = JsonValue::parse(json_str);
    
    EXPECT_TRUE(parsed.is_object());
    EXPECT_TRUE(parsed["setPartial"].as_bool());
    EXPECT_FALSE(parsed["enabled"].as_bool());
}

TEST_F(JsonSerializationTest, ParseOrNull) {
    std::string valid_json = "{\"test\":\"value\"}";
    std::string invalid_json = "{invalid json";
    
    JsonValue valid_parsed = JsonValue::parse_or_null(valid_json);
    JsonValue invalid_parsed = JsonValue::parse_or_null(invalid_json);
    
    EXPECT_TRUE(valid_parsed.is_object());
    EXPECT_TRUE(invalid_parsed.is_null());
}
