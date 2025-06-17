#include "TestCIMediator.hpp"
#include "TestPropertyRules.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include <memory>

using namespace midi_ci::core;

TestCIMediator::TestCIMediator() {
    device1_ = std::make_unique<MidiCIDevice>(19474 & 0x7F7F7F7F);
    device2_ = std::make_unique<MidiCIDevice>(37564 & 0x7F7F7F7F);
    
    device1Sender_ = [this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        device2_->processInput(group, data);
        return true;
    };
    
    device2Sender_ = [this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        device1_->processInput(group, data);
        return true;
    };
    
    device1_->set_sysex_sender(device1Sender_);
    device2_->set_sysex_sender(device2Sender_);
    
    device1_->initialize();
    device2_->initialize();
    
    auto property_rules1 = std::make_unique<TestPropertyRules>();
    auto property_rules2 = std::make_unique<TestPropertyRules>();
    
    device1_->get_property_host_facade().set_property_rules(std::move(property_rules1));
    device2_->get_property_host_facade().set_property_rules(std::move(property_rules2));
}
