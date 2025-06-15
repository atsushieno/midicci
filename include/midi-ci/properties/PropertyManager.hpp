#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>

namespace midi_ci {
namespace properties {

struct PropertyMetadata {
    std::string property_id;
    std::string resource_id;
    std::string name;
    std::string media_type;
    std::string encoding;
    std::vector<uint8_t> data;
    
    PropertyMetadata(const std::string& id, const std::string& res_id, const std::string& prop_name,
                    const std::string& type, const std::string& enc, const std::vector<uint8_t>& prop_data);
};

struct SubscriptionEntry {
    uint32_t muid;
    std::string resource;
    std::string subscribe_id;
    std::string encoding;
    
    SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                     const std::string& sub_id, const std::string& enc);
};

class PropertyManager {
public:
    using PropertyUpdateCallback = std::function<void(const std::string&, const std::vector<uint8_t>&, bool)>;
    using SubscriptionCallback = std::function<void(const SubscriptionEntry&)>;
    
    explicit PropertyManager();
    ~PropertyManager();
    
    PropertyManager(const PropertyManager&) = delete;
    PropertyManager& operator=(const PropertyManager&) = delete;
    
    PropertyManager(PropertyManager&&) = default;
    PropertyManager& operator=(PropertyManager&&) = default;
    
    void add_property(const PropertyMetadata& property);
    void remove_property(const std::string& property_id);
    void update_property_metadata(const std::string& old_property_id, const PropertyMetadata& property);
    
    void set_property_value(const std::string& property_id, const std::string& resource_id,
                           const std::vector<uint8_t>& data, bool is_partial);
    
    std::vector<PropertyMetadata> get_metadata_list() const;
    PropertyMetadata* find_property(const std::string& property_id);
    const PropertyMetadata* find_property(const std::string& property_id) const;
    
    std::vector<uint8_t> get_property_value(const std::string& property_id, const std::string& resource_id) const;
    
    void add_subscription(const SubscriptionEntry& subscription);
    void remove_subscription(uint32_t muid, const std::string& property_id);
    void remove_all_subscriptions(uint32_t muid);
    std::vector<SubscriptionEntry> get_subscriptions() const;
    std::vector<SubscriptionEntry> get_subscriptions_for_property(const std::string& property_id) const;
    
    void notify_property_update(const std::string& property_id, const std::vector<uint8_t>& data, bool is_partial);
    
    void set_property_update_callback(PropertyUpdateCallback callback);
    void add_subscription_callback(SubscriptionCallback callback);
    void remove_subscription_callback(SubscriptionCallback callback);
    
    uint8_t get_max_simultaneous_requests() const noexcept;
    void set_max_simultaneous_requests(uint8_t max_requests);
    
    std::vector<uint8_t> create_property_header(const std::string& property_id, 
                                              const std::map<std::string, std::string>& headers) const;
    std::string get_property_id_from_header(const std::vector<uint8_t>& header) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace properties
} // namespace midi_ci
