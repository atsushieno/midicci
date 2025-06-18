#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <utility>

namespace midicci {
namespace properties {

class PropertyChunkManager {
public:
    PropertyChunkManager();
    ~PropertyChunkManager();
    
    PropertyChunkManager(const PropertyChunkManager&) = delete;
    PropertyChunkManager& operator=(const PropertyChunkManager&) = delete;
    
    PropertyChunkManager(PropertyChunkManager&&) = default;
    PropertyChunkManager& operator=(PropertyChunkManager&&) = default;
    
    void add_pending_chunk(uint64_t timestamp, uint32_t source_muid, uint8_t request_id,
                          const std::vector<uint8_t>& header, const std::vector<uint8_t>& data);
    
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> 
    finish_pending_chunk(uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& final_data);
    
    bool has_pending_chunk(uint32_t source_muid, uint8_t request_id) const;
    
    void cleanup_expired_chunks(uint64_t current_timestamp, uint64_t timeout_seconds = 30);
    
    void clear_all_chunks();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace properties
} // namespace midi_ci
