#include <gtest/gtest.h>
#include <midicci/midicci.hpp>  // was: midicci/Json.hpp"

using namespace midicci;

TEST(JsonTest, parseString) {
    auto str1 = JsonValue::parse("\"TEST1\"");
    EXPECT_TRUE(str1.isString());
    EXPECT_EQ("TEST1", str1.asString());

    auto str2 = JsonValue::parse("\"TEST2\\\\r\\n\\t\\/\\b\\f\\u1234\\uFEDC\"");
    EXPECT_TRUE(str2.isString());
    EXPECT_EQ("TEST2\r\n\t/\b\u000c\u1234\uFEDC", str2.asString());
}

TEST(JsonTest, parseNull) {
    auto nullVal = JsonValue::parse("null");
    EXPECT_TRUE(nullVal.isNull());
}

TEST(JsonTest, parseBoolean) {
    auto trueVal = JsonValue::parse("true");
    EXPECT_TRUE(trueVal.isBool());
    EXPECT_TRUE(trueVal.asBool());

    auto falseVal = JsonValue::parse("false");
    EXPECT_TRUE(falseVal.isBool());
    EXPECT_FALSE(falseVal.asBool());
}

TEST(JsonTest, parseNumber) {
    auto num1 = JsonValue::parse("0");
    EXPECT_TRUE(num1.isNumber());
    EXPECT_EQ(0.0, num1.asNumber());

    auto num2 = JsonValue::parse("10");
    EXPECT_EQ(10.0, num2.asNumber());
    auto num3 = JsonValue::parse("10.0");
    EXPECT_EQ(10.0, num3.asNumber());
    auto num4 = JsonValue::parse("-1");
    EXPECT_EQ(-1.0, num4.asNumber());
    auto num5 = JsonValue::parse("-0");
    EXPECT_EQ(-0.0, num5.asNumber());
    auto num6 = JsonValue::parse("0.1");
    EXPECT_EQ(0.1, num6.asNumber());
    auto num7 = JsonValue::parse("-0.1");
    EXPECT_EQ(-0.1, num7.asNumber());
    auto num8 = JsonValue::parse("-0.1e12");
    EXPECT_EQ(-0.1e12, num8.asNumber());
    auto num9 = JsonValue::parse("-0.1e-12");
    EXPECT_EQ(-0.1e-12, num9.asNumber());
    auto num10 = JsonValue::parse("-0e-12");
    EXPECT_EQ(-0e-12, num10.asNumber());
    auto num11 = JsonValue::parse("1e+1");
    EXPECT_EQ(1e+1, num11.asNumber());
}

TEST(JsonTest, parseObject) {
    auto obj1 = JsonValue::parse("{}");
    EXPECT_TRUE(obj1.isObject());
    EXPECT_EQ(0, obj1.asObject().size());

    auto obj2 = JsonValue::parse("{ }");
    EXPECT_TRUE(obj2.isObject());
    EXPECT_EQ(0, obj2.asObject().size());
}

TEST(JsonTest, parseObject2) {
    auto obj2 = JsonValue::parse("{\"x,y\": 5, \"a,\\b\": 7}");
    EXPECT_TRUE(obj2.isObject());
    auto obj2Map = obj2.asObject();
    EXPECT_EQ(2, obj2Map.size());
    
    EXPECT_TRUE(obj2Map.find("x,y") != obj2Map.end());
    EXPECT_TRUE(obj2Map.at("x,y").isNumber());
    EXPECT_EQ(5.0, obj2Map.at("x,y").asNumber());
    
    EXPECT_TRUE(obj2Map.find("a,\b") != obj2Map.end());
    EXPECT_TRUE(obj2Map.at("a,\b").isNumber());
    EXPECT_EQ(7.0, obj2Map.at("a,\b").asNumber());
}

TEST(JsonTest, parseObject3) {
    auto obj3 = JsonValue::parse("{\"key1\": null, \"key2\": {\"key2-1\": true}, \"key3\": {\"key3-1\": {}, \"key3-2\": []} }");
    EXPECT_TRUE(obj3.isObject());
    EXPECT_EQ(3, obj3.asObject().size());
}

TEST(JsonTest, parseArray) {
    auto arr1 = JsonValue::parse("[]");
    EXPECT_TRUE(arr1.isArray());
    EXPECT_EQ(0, arr1.asArray().size());

    auto arr2 = JsonValue::parse("[ ]");
    EXPECT_TRUE(arr2.isArray());
    EXPECT_EQ(0, arr2.asArray().size());
}

TEST(JsonTest, parseArray2) {
    auto arr1 = JsonValue::parse("[1,2,3,4,5]");
    EXPECT_TRUE(arr1.isArray());
    auto arr1Items = arr1.asArray();
    EXPECT_EQ(5, arr1Items.size());
    EXPECT_TRUE(arr1Items[0].isNumber());
    EXPECT_EQ(1.0, arr1Items[0].asNumber());
    EXPECT_EQ(5.0, arr1Items[4].asNumber());
}

TEST(JsonTest, parseArray3) {
    auto arr2 = JsonValue::parse("[\"1\",2,[3,4],{\"x,y\": 5, \"a,\\b\": 7}, {\"\": {}}, \"{}[]\"]");
    EXPECT_TRUE(arr2.isArray());
    auto arr2Items = arr2.asArray();
    EXPECT_EQ(6, arr2Items.size());
    EXPECT_TRUE(arr2Items[0].isString());
    EXPECT_EQ("1", arr2Items[0].asString());
    EXPECT_TRUE(arr2Items[1].isNumber());
    EXPECT_TRUE(arr2Items[2].isArray());
    EXPECT_TRUE(arr2Items[3].isObject());
    EXPECT_TRUE(arr2Items[4].isObject());
    EXPECT_TRUE(arr2Items[5].isString());
    EXPECT_EQ("{}[]", arr2Items[5].asString());
}

TEST(JsonTest, parseArray4) {
    auto arr3 = JsonValue::parse("[{\"resource\":\"DeviceInfo\"},{\"resource\":\"foo\"},{\"resource\":\"bar\"}]");
    EXPECT_TRUE(arr3.isArray());
    auto arr3Items = arr3.asArray();
    EXPECT_EQ(3, arr3Items.size());
    for (size_t i = 0; i < arr3Items.size(); ++i) {
        EXPECT_TRUE(arr3Items[i].isObject());
        EXPECT_EQ(1, arr3Items[i].asObject().size());
    }
}
