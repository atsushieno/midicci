#include "TestCITransport.hpp"
#include <thread>

namespace midicci::test {

TestCITransport::TestCITransport() {
    config1_.device_info = {
        0x123456, 0x1234, 0x100, 0x00000001,
        "TestDevice1", "TestFamily1", "TestModel1", "1.0", "DEV1-001"
    };

    config2_.device_info = {
        0x654321, 0x4321, 0x200, 0x00000002,
        "TestDevice2", "TestFamily2", "TestModel2", "2.0", "DEV2-002"
    };

    device1_ = std::make_unique<MidiCIDevice>(19474 & 0x7F7F7F7F, config1_);
    device2_ = std::make_unique<MidiCIDevice>(37564 & 0x7F7F7F7F, config2_);

    device1_->setSysexSender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        if (device2_) {
            device2_->processInput(group, data);
        }
        return true;
    });

    device2_->setSysexSender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        if (device1_) {
            device1_->processInput(group, data);
        }
        return true;
    });
}

bool TestCITransport::waitForCondition(std::function<bool()> condition,
                                      std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    const auto poll_interval = std::chrono::milliseconds(10);

    while (std::chrono::steady_clock::now() - start < timeout) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(poll_interval);
    }

    return condition();
}

} // namespace midicci::test
