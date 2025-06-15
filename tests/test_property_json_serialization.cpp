#include <gtest/gtest.h>
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/json/Json.hpp"

using namespace midi_ci::messages;
using namespace midi_ci::json;

class PropertyJsonSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    Common common_{0x12345678, 0x87654321, 0, 0};
};

TEST_F(PropertyJsonSerializationTest, GetPropertyDataJsonHeader) {
    GetPropertyData msg(common_, 0x42, "DeviceInfo", "device1");
    
    auto packets = msg.serialize_multi();
    ASSERT_FALSE(packets.empty());
    
    const auto& header = msg.get_header();
    ASSERT_FALSE(header.empty());
    
    std::string header_str(header.begin(), header.end());
    auto json_val = JsonValue::parse_or_null(header_str);
    ASSERT_TRUE(json_val.is_object());
    
    EXPECT_EQ(json_val["resource"].as_string(), "DeviceInfo");
    EXPECT_EQ(json_val["resId"].as_string(), "device1");
}

TEST_F(PropertyJsonSerializationTest, SetPropertyDataJsonHeader) {
    std::vector<uint8_t> body_data = {0x01, 0x02, 0x03, 0x04};
    SetPropertyData msg(common_, 0x43, "Configuration", body_data, "config1", true);
    
    auto packets = msg.serialize_multi();
    ASSERT_FALSE(packets.empty());
    
    const auto& header = msg.get_header();
    ASSERT_FALSE(header.empty());
    
    std::string header_str(header.begin(), header.end());
    auto json_val = JsonValue::parse_or_null(header_str);
    ASSERT_TRUE(json_val.is_object());
    
    EXPECT_EQ(json_val["resource"].as_string(), "Configuration");
    EXPECT_EQ(json_val["resId"].as_string(), "config1");
    EXPECT_TRUE(json_val["setPartial"].as_bool());
}

TEST_F(PropertyJsonSerializationTest, SubscribePropertyJsonHeader) {
    SubscribeProperty msg(common_, 0x44, "DeviceInfo", "start", "ASCII");
    
    auto packets = msg.serialize_multi();
    ASSERT_FALSE(packets.empty());
    
    const auto& header = msg.get_header();
    ASSERT_FALSE(header.empty());
    
    std::string header_str(header.begin(), header.end());
    auto json_val = JsonValue::parse_or_null(header_str);
    ASSERT_TRUE(json_val.is_object());
    
    EXPECT_EQ(json_val["resource"].as_string(), "DeviceInfo");
    EXPECT_EQ(json_val["command"].as_string(), "start");
    EXPECT_EQ(json_val["mutualEncoding"].as_string(), "ASCII");
}

TEST_F(PropertyJsonSerializationTest, MultiPacketChunking) {
    std::vector<uint8_t> large_body(1000, 0xAB);
    SetPropertyData msg(common_, 0x45, "LargeData", large_body);
    
    auto packets = msg.serialize_multi();
    EXPECT_GT(packets.size(), 1);
    
    for (const auto& packet : packets) {
        EXPECT_GT(packet.size(), 20);
    }
}

TEST_F(PropertyJsonSerializationTest, JsonValueSerialization) {
    JsonValue json_obj = JsonValue::empty_object();
    json_obj["resource"] = JsonValue("TestResource");
    json_obj["resId"] = JsonValue("test123");
    json_obj["setPartial"] = JsonValue(true);
    json_obj["offset"] = JsonValue(10);
    
    auto serialized_bytes = json_obj.get_serialized_bytes();
    ASSERT_FALSE(serialized_bytes.empty());
    
    std::string json_str(serialized_bytes.begin(), serialized_bytes.end());
    auto parsed = JsonValue::parse(json_str);
    
    EXPECT_EQ(parsed["resource"].as_string(), "TestResource");
    EXPECT_EQ(parsed["resId"].as_string(), "test123");
    EXPECT_TRUE(parsed["setPartial"].as_bool());
    EXPECT_EQ(parsed["offset"].as_int(), 10);
}
