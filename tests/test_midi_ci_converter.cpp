#include <gtest/gtest.h>
#include <string>
#include <algorithm>

class MidiCIConverterTest : public ::testing::Test {
protected:
    std::string encodeStringToASCII(const std::string& input) {
        std::string result = input;
        size_t pos = 0;
        while ((pos = result.find("\\u", pos)) != std::string::npos) {
            result.replace(pos, 2, "\\u005cu");
            pos += 7;
        }
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    std::string decodeASCIIToString(const std::string& input) {
        std::string result = input;
        size_t pos = 0;
        while ((pos = result.find("\\u005cu", pos)) != std::string::npos) {
            result.replace(pos, 7, "\\u");
            pos += 2;
        }
        return result;
    }
};

TEST_F(MidiCIConverterTest, encodeStringToASCII) {
    std::string expected = "test\\u005cu#$%&'";
    std::string actual = encodeStringToASCII("test\\u#$%&'");
    EXPECT_EQ(expected, actual);
}

TEST_F(MidiCIConverterTest, decodeStringToASCII) {
    std::string expected = "test\\u#$%&'";
    std::string actual = decodeASCIIToString("test\\u005cu#$%&'");
    EXPECT_EQ(expected, actual);
}
