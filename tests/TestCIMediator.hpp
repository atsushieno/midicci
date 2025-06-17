#pragma once

#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/properties/PropertyHostFacade.hpp"
#include "midi-ci/profiles/ProfileHostFacade.hpp"
#include <memory>
#include <vector>
#include <cstdint>

using namespace midi_ci::core;

class TestPropertyRules;

class TestCIMediator {
public:
    TestCIMediator();
    ~TestCIMediator() = default;
    
    MidiCIDevice& getDevice1() { return *device1_; }
    MidiCIDevice& getDevice2() { return *device2_; }
    
private:
    std::unique_ptr<MidiCIDevice> device1_;
    std::unique_ptr<MidiCIDevice> device2_;
    
    MidiCIDevice::CIOutputSender device1Sender_;
    MidiCIDevice::CIOutputSender device2Sender_;
};
