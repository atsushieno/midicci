#include <gtest/gtest.h>
#include <midicci/midicci.hpp>  // was: midicci/Json.hpp"

using namespace midicci;

class JsonSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(JsonSerializationTest, EmptyObject) {
    JsonValue obj = JsonValue::emptyObject();
    EXPECT_TRUE(obj.isObject());
    EXPECT_EQ(obj.serialize(), "{}");
}

TEST_F(JsonSerializationTest, SimpleObject) {
    JsonValue obj = JsonValue::emptyObject();
    obj["resource"] = JsonValue("DeviceInfo");
    obj["resId"] = JsonValue("device1");
    
    std::string json_str = obj.serialize();
    EXPECT_TRUE(json_str.find("\"resource\":\"DeviceInfo\"") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"resId\":\"device1\"") != std::string::npos);
}

TEST_F(JsonSerializationTest, BooleanValues) {
    JsonValue obj = JsonValue::emptyObject();
    obj["setPartial"] = JsonValue(true);
    obj["enabled"] = JsonValue(false);
    
    std::string json_str = obj.serialize();
    EXPECT_TRUE(json_str.find("\"setPartial\":true") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"enabled\":false") != std::string::npos);
}

TEST_F(JsonSerializationTest, NumericValues) {
    JsonValue obj = JsonValue::emptyObject();
    obj["offset"] = JsonValue(42);
    obj["limit"] = JsonValue(100);
    
    std::string json_str = obj.serialize();
    EXPECT_TRUE(json_str.find("\"offset\":42") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"limit\":100") != std::string::npos);
}

TEST_F(JsonSerializationTest, SerializedBytes) {
    JsonValue obj = JsonValue::emptyObject();
    obj["resource"] = JsonValue("TestResource");
    obj["value"] = JsonValue(123);
    
    auto bytes = obj.getSerializedBytes();
    EXPECT_FALSE(bytes.empty());
    
    std::string json_str(bytes.begin(), bytes.end());
    EXPECT_TRUE(json_str.find("TestResource") != std::string::npos);
    EXPECT_TRUE(json_str.find("123") != std::string::npos);
}

TEST_F(JsonSerializationTest, ParseSimpleObject) {
    std::string json_str = "{\"resource\":\"DeviceInfo\",\"resId\":\"device1\"}";
    JsonValue parsed = JsonValue::parse(json_str);
    
    EXPECT_TRUE(parsed.isObject());
    EXPECT_EQ(parsed["resource"].asString(), "DeviceInfo");
    EXPECT_EQ(parsed["resId"].asString(), "device1");
}

TEST_F(JsonSerializationTest, ParseWithBooleans) {
    std::string json_str = "{\"setPartial\":true,\"enabled\":false}";
    JsonValue parsed = JsonValue::parse(json_str);
    
    EXPECT_TRUE(parsed.isObject());
    EXPECT_TRUE(parsed["setPartial"].asBool());
    EXPECT_FALSE(parsed["enabled"].asBool());
}

TEST_F(JsonSerializationTest, ParseOrNull) {
    std::string valid_json = "{\"test\":\"value\"}";
    std::string invalid_json = "{invalid json_ish";
    
    JsonValue valid_parsed = JsonValue::parseOrNull(valid_json);
    JsonValue invalid_parsed = JsonValue::parseOrNull(invalid_json);
    
    EXPECT_TRUE(valid_parsed.isObject());
    EXPECT_TRUE(invalid_parsed.isNull());
}
