#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include "midicci/midicci.hpp"

namespace midicci {
class ClientConnection;
struct ClientSubscription;
class MidiCIClientPropertyRules;

namespace commonproperties {
class PropertyMetadata;
}

struct PropertySubscription;
class ClientObservablePropertyList;

class PropertyClientFacade {
public:
    using SubscriptionUpdateCallback = std::function<void(const ClientSubscription&)>;
    using GetPropertyDataCallback = std::function<void(const GetPropertyDataReply&)>;
    using SetPropertyDataCallback = std::function<void(const SetPropertyDataReply&)>;

    PropertyClientFacade(MidiCIDevice& device, ClientConnection& conn);
    ~PropertyClientFacade();

    PropertyClientFacade(const PropertyClientFacade&) = delete;
    PropertyClientFacade& operator=(const PropertyClientFacade&) = delete;

    PropertyClientFacade(PropertyClientFacade&&) = default;
    PropertyClientFacade& operator=(PropertyClientFacade&&) = default;

    void setPropertyRules(std::unique_ptr<MidiCIClientPropertyRules> rules);
    MidiCIClientPropertyRules* getPropertyRules();

    uint8_t sendGetPropertyData(const std::string& resource, const std::string& res_id, const std::string& encoding = "", int paginate_offset = -1, int paginate_limit = -1);
    void sendGetPropertyData(const GetPropertyData& msg);

    uint8_t sendSetPropertyData(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, const std::string& encoding = "", bool is_partial = false);
    void sendSetPropertyData(const SetPropertyData& msg);

    void getPropertyData(const std::string& resource, const std::string& res_id, GetPropertyDataCallback callback, const std::string& encoding = "", int paginate_offset = -1, int paginate_limit = -1);
    void setPropertyData(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, SetPropertyDataCallback callback, const std::string& encoding = "", bool is_partial = false);
    
    void sendSubscribeProperty(const std::string& resource, const std::string& res_id, const std::string& mutual_encoding = "", const std::string& subscription_id = "");
    void sendUnsubscribeProperty(const std::string& property_id, const std::string& res_id);
    
    void processPropertyCapabilitiesReply(const PropertyGetCapabilitiesReply& msg);
    void processGetDataReply(const GetPropertyDataReply& msg);
    void processSetDataReply(const SetPropertyDataReply& msg);
    SubscribePropertyReply processSubscribeProperty(const SubscribeProperty& msg);
    void processSubscribePropertyReply(const SubscribePropertyReply& msg);
    
    std::vector<ClientSubscription> getSubscriptions() const;
    ClientObservablePropertyList* getProperties();
    
    PropertyChunkManager& getPendingChunkManager();
    const PropertyChunkManager& getPendingChunkManager() const;
    
    void addSubscriptionUpdateCallback(SubscriptionUpdateCallback callback);
    void removeSubscriptionUpdateCallback(const SubscriptionUpdateCallback& callback);

private:
    SubscribePropertyReply handleUnsubscriptionNotification(const SubscribeProperty& msg);
    std::pair<std::string, SubscribePropertyReply> updatePropertyBySubscribe(const SubscribeProperty& msg);
    void addPendingSubscription(uint8_t request_id, const std::string& subscription_id, const std::string& property_id, const std::string& res_id);
    void promoteSubscriptionAsUnsubscribing(const std::string& property_id, const std::string& res_id, uint8_t new_request_id);
    void notifySubscriptionUpdated(const ClientSubscription& subscription);
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class MidiCIClientPropertyRules {
public:
    virtual ~MidiCIClientPropertyRules() = default;
    
    virtual std::vector<uint8_t> createDataRequestHeader(const std::string& resource, const std::map<std::string, std::string>& fields) = 0;
    virtual std::vector<uint8_t> createSubscriptionHeader(const std::string& resource, const std::map<std::string, std::string>& fields) = 0;
    virtual std::vector<uint8_t> createStatusHeader(int status) = 0;
    virtual std::vector<uint8_t> encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) = 0;
    virtual std::vector<uint8_t> decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) = 0;
    virtual std::string getPropertyIdForHeader(const std::vector<uint8_t>& header) = 0;
    virtual std::string getResIdForHeader(const std::vector<uint8_t>& header) = 0;
    virtual std::string getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) = 0;
    virtual int getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) = 0;
    virtual void processPropertySubscriptionResult(void* sub, const SubscribePropertyReply& msg) = 0;
    virtual void propertyValueUpdated(const std::string& property_id, const std::vector<uint8_t>& body) = 0;
    virtual void requestPropertyList(uint8_t group) = 0;
    virtual std::string getSubscribedProperty(const SubscribeProperty& msg) = 0;
};

} // namespace
