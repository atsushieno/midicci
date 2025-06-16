#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace midi_ci {
namespace core {
class MidiCIDevice;
}
namespace properties {

class CommonRulesPropertyHelper {
public:
    explicit CommonRulesPropertyHelper(core::MidiCIDevice& device);
    
    std::vector<uint8_t> create_request_header_bytes(const std::string& property_id, 
                                                    const std::map<std::string, std::string>& fields) const;
    
    std::vector<uint8_t> create_subscribe_header_bytes(const std::string& property_id, 
                                                      const std::string& command, 
                                                      const std::string& mutual_encoding = "") const;
    
    std::string get_property_identifier_internal(const std::vector<uint8_t>& header) const;
    
    std::vector<uint8_t> get_resource_list_request_bytes() const;
    
    std::string get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) const;
    int get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) const;
    
    std::vector<uint8_t> encode_body(const std::vector<uint8_t>& data, const std::string& encoding) const;
    std::vector<uint8_t> decode_body(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) const;

private:
    core::MidiCIDevice& device_;
};

} // namespace properties
} // namespace midi_ci
