#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include "Message.hpp"
#include "MidiCIDevice.hpp"

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
    PropertyClientFacade(MidiCIDevice& device, ClientConnection& conn);
    ~PropertyClientFacade();
    
    PropertyClientFacade(const PropertyClientFacade&) = delete;
    PropertyClientFacade& operator=(const PropertyClientFacade&) = delete;
    
    PropertyClientFacade(PropertyClientFacade&&) = default;
    PropertyClientFacade& operator=(PropertyClientFacade&&) = default;
    
    void set_property_rules(std::unique_ptr<MidiCIClientPropertyRules> rules);
    MidiCIClientPropertyRules* get_property_rules();
    
    void send_get_property_data(const std::string& resource, const std::string& encoding = "", int paginate_offset = -1, int paginate_limit = -1);
    void send_get_property_data(const GetPropertyData& msg);
    
    void send_set_property_data(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, const std::string& encoding = "", bool is_partial = false);
    void send_set_property_data(const SetPropertyData& msg);
    
    void send_subscribe_property(const std::string& resource, const std::string& mutual_encoding = "", const std::string& subscription_id = "");
    void send_unsubscribe_property(const std::string& property_id);
    
    void process_property_capabilities_reply(const PropertyGetCapabilitiesReply& msg);
    void process_get_data_reply(const GetPropertyDataReply& msg);
    void process_set_data_reply(const SetPropertyDataReply& msg);
    void process_subscribe_property(const SubscribeProperty& msg);
    void process_subscribe_property_reply(const SubscribePropertyReply& msg);
    
    std::vector<ClientSubscription> get_subscriptions() const;
    ClientObservablePropertyList* get_properties();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class MidiCIClientPropertyRules {
public:
    virtual ~MidiCIClientPropertyRules() = default;
    
    virtual std::vector<uint8_t> create_data_request_header(const std::string& resource, const std::map<std::string, std::string>& fields) = 0;
    virtual std::vector<uint8_t> create_subscription_header(const std::string& resource, const std::map<std::string, std::string>& fields) = 0;
    virtual std::vector<uint8_t> create_status_header(int status) = 0;
    virtual std::vector<uint8_t> encode_body(const std::vector<uint8_t>& data, const std::string& encoding) = 0;
    virtual std::string get_property_id_for_header(const std::vector<uint8_t>& header) = 0;
    virtual std::string get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) = 0;
    virtual int get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) = 0;
    virtual void process_property_subscription_result(void* sub, const SubscribePropertyReply& msg) = 0;
    virtual void property_value_updated(const std::string& property_id, const std::vector<uint8_t>& body) = 0;
    virtual void request_property_list(uint8_t group) = 0;
};

} // namespace
