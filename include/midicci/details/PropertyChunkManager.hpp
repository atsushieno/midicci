#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <utility>

namespace midicci {

class PropertyChunkManager {
public:
    PropertyChunkManager();
    ~PropertyChunkManager();
    
    PropertyChunkManager(const PropertyChunkManager&) = delete;
    PropertyChunkManager& operator=(const PropertyChunkManager&) = delete;
    
    PropertyChunkManager(PropertyChunkManager&&) = default;
    PropertyChunkManager& operator=(PropertyChunkManager&&) = default;
    
    void addPendingChunk(uint64_t timestamp, uint32_t source_muid, uint8_t request_id,
                          const std::vector<uint8_t>& header, const std::vector<uint8_t>& data);
    
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> 
    finishPendingChunk(uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& final_data);
    
    bool hasPendingChunk(uint32_t source_muid, uint8_t request_id) const;

    std::vector<uint8_t> getPendingHeader(uint32_t source_muid, uint8_t request_id) const;
    
    void cleanupExpiredChunks(uint64_t current_timestamp, uint64_t timeout_seconds = 30);
    
    void clearAllChunks();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
