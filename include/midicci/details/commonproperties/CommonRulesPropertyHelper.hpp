#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include "midicci/midicci.hpp"

namespace midicci {
namespace commonproperties {

class CommonRulesPropertyHelper {
public:
    explicit CommonRulesPropertyHelper(MidiCIDevice& device);
    
    std::vector<uint8_t> createRequestHeaderBytes(const std::string& property_id, 
                                                    const std::map<std::string, std::string>& fields) const;
    
    std::vector<uint8_t> createSubscribeHeaderBytes(const std::string& property_id, 
                                                      const std::string& command, 
                                                      const std::string& mutual_encoding = "") const;
    
    std::string getPropertyIdentifierInternal(const std::vector<uint8_t>& header) const;
    
    std::vector<uint8_t> getResourceListRequestBytes() const;
    
    std::string getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) const;
    int getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) const;
    
    std::vector<uint8_t> encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) const;
    std::vector<uint8_t> decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) const;
    std::vector<uint8_t> decodeBody(const std::optional<std::string>& encoding, const std::vector<uint8_t>& body) const;

private:
    MidiCIDevice& device_;
};

} // namespace properties
} // namespace midi_ci
