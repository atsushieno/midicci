#include "midicci/PropertyChunkManager.hpp"
#include <mutex>
#include <vector>
#include <algorithm>

namespace midicci {

struct Chunk {
    uint64_t timestamp;
    uint32_t source_muid;
    uint8_t request_id;
    std::vector<uint8_t> header;
    std::vector<uint8_t> data;
    
    Chunk(uint64_t ts, uint32_t muid, uint8_t req_id, const std::vector<uint8_t>& hdr, const std::vector<uint8_t>& chunk_data)
        : timestamp(ts), source_muid(muid), request_id(req_id), header(hdr), data(chunk_data) {}
};

class PropertyChunkManager::Impl {
public:
    std::vector<Chunk> chunks_;
    mutable std::recursive_mutex mutex_;
};

PropertyChunkManager::PropertyChunkManager() : pimpl_(std::make_unique<Impl>()) {}

PropertyChunkManager::~PropertyChunkManager() = default;

void PropertyChunkManager::add_pending_chunk(uint64_t timestamp, uint32_t source_muid, uint8_t request_id,
                                            const std::vector<uint8_t>& header, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->chunks_.begin(), pimpl_->chunks_.end(),
        [&](const Chunk& chunk) {
            return chunk.source_muid == source_muid && chunk.request_id == request_id;
        });
    
    if (it != pimpl_->chunks_.end()) {
        it->data.insert(it->data.end(), data.begin(), data.end());
    } else {
        pimpl_->chunks_.emplace_back(timestamp, source_muid, request_id, header, data);
    }
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> 
PropertyChunkManager::finish_pending_chunk(uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& final_data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->chunks_.begin(), pimpl_->chunks_.end(),
        [&](const Chunk& chunk) {
            return chunk.source_muid == source_muid && chunk.request_id == request_id;
        });
    
    if (it != pimpl_->chunks_.end()) {
        it->data.insert(it->data.end(), final_data.begin(), final_data.end());
        
        std::pair<std::vector<uint8_t>, std::vector<uint8_t>> result = {it->header, it->data};
        pimpl_->chunks_.erase(it);
        return result;
    }
    
    return {{}, final_data};
}

bool PropertyChunkManager::has_pending_chunk(uint32_t source_muid, uint8_t request_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->chunks_.begin(), pimpl_->chunks_.end(),
        [&](const Chunk& chunk) {
            return chunk.source_muid == source_muid && chunk.request_id == request_id;
        });
    
    return it != pimpl_->chunks_.end();
}

void PropertyChunkManager::cleanup_expired_chunks(uint64_t current_timestamp, uint64_t timeout_seconds) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::remove_if(pimpl_->chunks_.begin(), pimpl_->chunks_.end(),
        [&](const Chunk& chunk) {
            return (current_timestamp - chunk.timestamp) > timeout_seconds;
        });
    
    pimpl_->chunks_.erase(it, pimpl_->chunks_.end());
}

void PropertyChunkManager::clear_all_chunks() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->chunks_.clear();
}

} // namespace
