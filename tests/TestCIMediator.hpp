#pragma once

#include "midicci/core/MidiCIDevice.hpp"
#include "midicci/core/ClientConnection.hpp"
#include "midicci/properties/PropertyHostFacade.hpp"
#include "midicci/profiles/ProfileHostFacade.hpp"
#include "midicci/core/MidiCIDeviceConfiguration.hpp"
#include <memory>
#include <vector>
#include <cstdint>

using namespace midicci::core;

class TestPropertyRules;

class TestCIMediator {
public:
    TestCIMediator();
    ~TestCIMediator() = default;
    
    MidiCIDevice& getDevice1() { return *device1_; }
    MidiCIDevice& getDevice2() { return *device2_; }
    
private:
    MidiCIDeviceConfiguration config_{};
    std::unique_ptr<MidiCIDevice> device1_;
    std::unique_ptr<MidiCIDevice> device2_;
    
    MidiCIDevice::CIOutputSender device1Sender_;
    MidiCIDevice::CIOutputSender device2Sender_;
};
