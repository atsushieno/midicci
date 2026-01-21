#pragma once

#include <umppi/details/Midi1Message.hpp>
#include <array>
#include <vector>
#include <functional>
#include <cstdint>

namespace umppi {

enum class DteTarget {
    RPN,
    NRPN
};

class Midi1SystemCommon {
public:
    uint8_t mtcQuarterFrame = 0;
    int16_t songPositionPointer = 0;
    uint8_t songSelect = 0;
};

class Midi1ControllerCatalog {
public:
    std::array<bool, 0x80 * 0x80> enabledRpns;
    std::array<bool, 0x80 * 0x80> enabledNrpns;

    Midi1ControllerCatalog();

    void enableAllNrpnMsbs();
};

class Midi1MachineChannel {
public:
    std::array<bool, 128> noteOnStatus;
    std::array<uint8_t, 128> noteVelocity;
    std::array<uint8_t, 128> pafVelocity;
    std::array<uint8_t, 128> controls;

    bool omniMode = false;
    bool monoPolyMode = true;

    std::array<int16_t, 128 * 128> rpns;
    std::array<int16_t, 128 * 128> nrpns;

    uint8_t program = 0;
    uint8_t caf = 0;
    int16_t pitchbend = 8192;

    DteTarget dteTarget = DteTarget::RPN;

    Midi1MachineChannel();

    int getCurrentRPN() const;
    int getCurrentNRPN() const;

    void processDte(uint8_t value, bool isMsb);
    void processDteIncrement();
    void processDteDecrement();
};

class Midi1Machine {
public:
    using OnMidi1MessageListener = std::function<void(const Midi1Message&)>;

    std::vector<OnMidi1MessageListener> messageListeners;
    Midi1ControllerCatalog controllerCatalog;
    Midi1SystemCommon systemCommon;
    std::array<Midi1MachineChannel, 16> channels;

    void processMessage(const Midi1Message& message);
};

}
