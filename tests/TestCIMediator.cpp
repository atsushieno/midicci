#include "TestCIMediator.hpp"
#include <memory>

using namespace midicci;

TestCIMediator::TestCIMediator() {
    config_.device_info = {0, 0, 0, 0, "TestDevice", "TestInitiatorFamily", "TestInitiatorModel", "0.0", "ABCDEFGH"};
    device1_ = std::make_unique<MidiCIDevice>(19474 & 0x7F7F7F7F, config_);
    device2_ = std::make_unique<MidiCIDevice>(37564 & 0x7F7F7F7F, config_);
    
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
}
