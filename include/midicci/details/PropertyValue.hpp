#pragma once

#include "midicci/midicci.hpp"

namespace midicci {
    struct PropertyValue {
        std::string id;
        std::string resId;
        std::string mediaType;
        std::vector<uint8_t> body;

        PropertyValue(const std::string& property_id, const std::string& media_type, const std::vector<uint8_t>& data)
                : id(property_id), resId(""), mediaType(media_type), body(data) {}

        PropertyValue(const std::string& property_id, const std::string& resource_id, const std::string& media_type, const std::vector<uint8_t>& data)
                : id(property_id), resId(resource_id), mediaType(media_type), body(data) {}

        bool operator==(const PropertyValue& other) const {
            return id == other.id && resId == other.resId && mediaType == other.mediaType && body == other.body;
        }
    };

    struct SubscriptionEntry {
        uint32_t muid;
        std::string resource;
        std::string res_id;
        std::string subscribe_id;
        std::string encoding;

        SubscriptionEntry(uint32_t subscriber_muid, const std::string& resource, const std::string& resid,
                          const std::string& sub_id, const std::string& enc);
    };

    using PropertyUpdatedCallback = std::function<void(const std::string&)>;
    using PropertyCatalogUpdatedCallback = std::function<void()>;
}