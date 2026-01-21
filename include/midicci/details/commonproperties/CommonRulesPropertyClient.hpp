#pragma once

#include "midicci/midicci.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace midicci::commonproperties {

class CommonRulesPropertyClient : public MidiCIClientPropertyRules {
public:
    CommonRulesPropertyClient(MidiCIDevice& device, ClientConnection& conn);
    ~CommonRulesPropertyClient() override = default;
    
    std::vector<uint8_t> createDataRequestHeader(const std::string& resource, 
                                                   const std::map<std::string, std::string>& fields) override;
    
    std::vector<uint8_t> createSubscriptionHeader(const std::string& resource, 
                                                   const std::map<std::string, std::string>& fields) override;
    
    std::vector<uint8_t> createStatusHeader(int status) override;
    
    std::vector<uint8_t> encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) override;

    std::vector<uint8_t> decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) override;

    std::string getPropertyIdForHeader(const std::vector<uint8_t>& header) override;
    
    std::string getResIdForHeader(const std::vector<uint8_t>& header) override;
    
    std::string getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) override;
    
    int getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) override;
    
    void processPropertySubscriptionResult(void* sub, const SubscribePropertyReply& msg) override;
    
    void propertyValueUpdated(const std::string& property_id, const std::vector<uint8_t>& body) override;
    
    void requestPropertyList(uint8_t group) override;
    
    std::string getSubscribedProperty(const SubscribeProperty& msg) override;
    
    void addPropertyCatalogUpdatedCallback(std::function<void()> callback);
    
    std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() const;

private:
    MidiCIDevice& device_;
    ClientConnection& conn_;
    std::unique_ptr<CommonRulesPropertyHelper> helper_;
    std::vector<std::function<void()>> property_catalog_updated_callbacks_;
    std::vector<std::unique_ptr<PropertyMetadata>> resource_list_;
    std::vector<SubscriptionEntry> subscriptions_;
    
    std::vector<std::unique_ptr<PropertyMetadata>> getMetadataListForBody(const std::vector<uint8_t>& body);
};

} // namespace
