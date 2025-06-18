#include <gtest/gtest.h>
#include "midi-ci/json/Json.hpp"

using namespace midi_ci::json;

TEST(JsonTest, parseString) {
    auto str1 = JsonValue::parse("\"TEST1\"");
    EXPECT_TRUE(str1.is_string());
    EXPECT_EQ("TEST1", str1.as_string());

    auto str2 = JsonValue::parse("\"TEST2\\\\r\\n\\t\\/\\b\\f\\u1234\\uFEDC\"");
    EXPECT_TRUE(str2.is_string());
    EXPECT_EQ("TEST2\\r\n\t/\b\f4\xDC", str2.as_string());
}

TEST(JsonTest, parseNull) {
    auto nullVal = JsonValue::parse("null");
    EXPECT_TRUE(nullVal.is_null());
}

TEST(JsonTest, parseBoolean) {
    auto trueVal = JsonValue::parse("true");
    EXPECT_TRUE(trueVal.is_bool());
    EXPECT_TRUE(trueVal.as_bool());

    auto falseVal = JsonValue::parse("false");
    EXPECT_TRUE(falseVal.is_bool());
    EXPECT_FALSE(falseVal.as_bool());
}

TEST(JsonTest, parseNumber) {
    auto num1 = JsonValue::parse("0");
    EXPECT_TRUE(num1.is_number());
    EXPECT_EQ(0.0, num1.as_number());

    auto num2 = JsonValue::parse("10");
    EXPECT_EQ(10.0, num2.as_number());
    auto num3 = JsonValue::parse("10.0");
    EXPECT_EQ(10.0, num3.as_number());
    auto num4 = JsonValue::parse("-1");
    EXPECT_EQ(-1.0, num4.as_number());
    auto num5 = JsonValue::parse("-0");
    EXPECT_EQ(-0.0, num5.as_number());
    auto num6 = JsonValue::parse("0.1");
    EXPECT_EQ(0.1, num6.as_number());
    auto num7 = JsonValue::parse("-0.1");
    EXPECT_EQ(-0.1, num7.as_number());
    auto num8 = JsonValue::parse("-0.1e12");
    EXPECT_EQ(-0.1e12, num8.as_number());
    auto num9 = JsonValue::parse("-0.1e-12");
    EXPECT_EQ(-0.1e-12, num9.as_number());
    auto num10 = JsonValue::parse("-0e-12");
    EXPECT_EQ(-0e-12, num10.as_number());
    auto num11 = JsonValue::parse("1e+1");
    EXPECT_EQ(1e+1, num11.as_number());
}

TEST(JsonTest, parseObject) {
    auto obj1 = JsonValue::parse("{}");
    EXPECT_TRUE(obj1.is_object());
    EXPECT_EQ(0, obj1.as_object().size());

    auto obj2 = JsonValue::parse("{ }");
    EXPECT_TRUE(obj2.is_object());
    EXPECT_EQ(0, obj2.as_object().size());
}

TEST(JsonTest, parseObject2) {
    auto obj2 = JsonValue::parse("{\"x,y\": 5, \"a,\\b\": 7}");
    EXPECT_TRUE(obj2.is_object());
    auto obj2Map = obj2.as_object();
    EXPECT_EQ(2, obj2Map.size());
    
    EXPECT_TRUE(obj2Map.find("x,y") != obj2Map.end());
    EXPECT_TRUE(obj2Map.at("x,y").is_number());
    EXPECT_EQ(5.0, obj2Map.at("x,y").as_number());
    
    EXPECT_TRUE(obj2Map.find("a,\b") != obj2Map.end());
    EXPECT_TRUE(obj2Map.at("a,\b").is_number());
    EXPECT_EQ(7.0, obj2Map.at("a,\b").as_number());
}

TEST(JsonTest, parseObject3) {
    auto obj3 = JsonValue::parse("{\"key1\": null, \"key2\": {\"key2-1\": true}, \"key3\": {\"key3-1\": {}, \"key3-2\": []} }");
    EXPECT_TRUE(obj3.is_object());
    EXPECT_EQ(3, obj3.as_object().size());
}

TEST(JsonTest, parseArray) {
    auto arr1 = JsonValue::parse("[]");
    EXPECT_TRUE(arr1.is_array());
    EXPECT_EQ(0, arr1.as_array().size());

    auto arr2 = JsonValue::parse("[ ]");
    EXPECT_TRUE(arr2.is_array());
    EXPECT_EQ(0, arr2.as_array().size());
}

TEST(JsonTest, parseArray2) {
    auto arr1 = JsonValue::parse("[1,2,3,4,5]");
    EXPECT_TRUE(arr1.is_array());
    auto arr1Items = arr1.as_array();
    EXPECT_EQ(5, arr1Items.size());
    EXPECT_TRUE(arr1Items[0].is_number());
    EXPECT_EQ(1.0, arr1Items[0].as_number());
    EXPECT_EQ(5.0, arr1Items[4].as_number());
}

TEST(JsonTest, parseArray3) {
    auto arr2 = JsonValue::parse("[\"1\",2,[3,4],{\"x,y\": 5, \"a,\\b\": 7}, {\"\": {}}, \"{}[]\"]");
    EXPECT_TRUE(arr2.is_array());
    auto arr2Items = arr2.as_array();
    EXPECT_EQ(6, arr2Items.size());
    EXPECT_TRUE(arr2Items[0].is_string());
    EXPECT_EQ("1", arr2Items[0].as_string());
    EXPECT_TRUE(arr2Items[1].is_number());
    EXPECT_TRUE(arr2Items[2].is_array());
    EXPECT_TRUE(arr2Items[3].is_object());
    EXPECT_TRUE(arr2Items[4].is_object());
    EXPECT_TRUE(arr2Items[5].is_string());
    EXPECT_EQ("{}[]", arr2Items[5].as_string());
}

TEST(JsonTest, parseArray4) {
    auto arr3 = JsonValue::parse("[{\"resource\":\"DeviceInfo\"},{\"resource\":\"foo\"},{\"resource\":\"bar\"}]");
    EXPECT_TRUE(arr3.is_array());
    auto arr3Items = arr3.as_array();
    EXPECT_EQ(3, arr3Items.size());
    for (size_t i = 0; i < arr3Items.size(); ++i) {
        EXPECT_TRUE(arr3Items[i].is_object());
        EXPECT_EQ(1, arr3Items[i].as_object().size());
    }
}
