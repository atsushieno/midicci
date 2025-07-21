#include "midicci/midicci.hpp"
#include <sstream>
#include <iomanip>

namespace midicci {

std::string MidiCIConverter::encodeStringToASCII(const std::string& s) {
    bool needsEncoding = false;
    for (char c : s) {
        if (static_cast<unsigned char>(c) >= 0x80 || c == '\\') {
            needsEncoding = true;
            break;
        }
    }
    
    if (!needsEncoding) {
        return s;
    }
    
    std::ostringstream result;
    for (char c : s) {
        if (static_cast<unsigned char>(c) < 0x80 && c != '\\') {
            result << c;
        } else {
            result << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                   << static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
    }
    return result.str();
}

std::string MidiCIConverter::decodeASCIIToString(const std::string& s) {
    std::string result;
    size_t pos = 0;
    
    size_t uPos = s.find("\\u");
    if (uPos == std::string::npos) {
        return s;
    }
    
    result += s.substr(0, uPos);
    pos = uPos;
    
    while (pos < s.length()) {
        if (pos + 5 < s.length() && s.substr(pos, 2) == "\\u") {
            std::string hexStr = s.substr(pos + 2, 4);
            unsigned int codepoint = std::stoul(hexStr, nullptr, 16);
            result += static_cast<char>(codepoint);
            pos += 6;
            
            size_t nextU = s.find("\\u", pos);
            if (nextU == std::string::npos) {
                result += s.substr(pos);
                break;
            } else {
                result += s.substr(pos, nextU - pos);
                pos = nextU;
            }
        } else {
            result += s[pos];
            pos++;
        }
    }
    
    return result;
}

} // namespace
