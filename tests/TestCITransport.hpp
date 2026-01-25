#pragma once

#include <midicci/midicci.hpp>
#include <memory>
#include <functional>
#include <chrono>

namespace midicci::test {

class TestCITransport {
public:
    TestCITransport();
    ~TestCITransport() = default;

    MidiCIDevice& getDevice1() { return *device1_; }
    MidiCIDevice& getDevice2() { return *device2_; }

    bool waitForCondition(std::function<bool()> condition,
                         std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

private:
    MidiCIDeviceConfiguration config1_;
    MidiCIDeviceConfiguration config2_;
    std::unique_ptr<MidiCIDevice> device1_;
    std::unique_ptr<MidiCIDevice> device2_;
};

} // namespace midicci::test
