#pragma once

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <optional>

#include "midicci/midicci.hpp"

namespace midicci {
namespace commonproperties {

class MidiCIServicePropertyRules {
public:
    virtual ~MidiCIServicePropertyRules() = default;
    
    virtual std::string getPropertyIdForHeader(const std::vector<uint8_t>& header) = 0;
    virtual std::vector<uint8_t> createUpdateNotificationHeader(const std::string& property_id, const std::map<std::string, std::string>& fields) = 0;
    virtual std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() = 0;
    
    virtual GetPropertyDataReply getPropertyData(const GetPropertyData& msg) = 0;
    virtual SetPropertyDataReply setPropertyData(const SetPropertyData& msg) = 0;
    virtual std::optional<SubscribePropertyReply> subscribeProperty(const SubscribeProperty& msg) = 0;
    
    virtual void addMetadata(std::unique_ptr<PropertyMetadata> property) = 0;
    virtual void removeMetadata(const std::string& property_id) = 0;
    
    virtual std::vector<uint8_t> encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) = 0;
    virtual std::vector<uint8_t> decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) = 0;
    virtual std::string getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) = 0;
    virtual int getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) = 0;
    virtual std::vector<uint8_t> createShutdownSubscriptionHeader(const std::string& property_id, const std::string& res_id) = 0;
    
    virtual const std::vector<SubscriptionEntry>& getSubscriptions() const = 0;
    
    void addPropertyCatalogUpdatedCallback(std::function<void()> callback);
    
protected:
    std::vector<std::function<void()>> property_catalog_updated_callbacks_;
};





} // namespace properties
} // namespace midi_ci
