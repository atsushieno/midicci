#include <umppi/details/Midi1Machine.hpp>
#include <umppi/details/Common.hpp>
#include <umppi/details/Utility.hpp>

namespace umppi {

namespace {
    std::array<bool, 0x80 * 0x80> createStandardRpnEnabled() {
        std::array<bool, 0x80 * 0x80> enabled = {};
        enabled[MidiRpn::PITCH_BEND_SENSITIVITY] = true;
        enabled[MidiRpn::FINE_TUNING] = true;
        enabled[MidiRpn::COARSE_TUNING] = true;
        enabled[MidiRpn::TUNING_PROGRAM] = true;
        enabled[MidiRpn::TUNING_BANK_SELECT] = true;
        enabled[MidiRpn::MODULATION_DEPTH] = true;
        return enabled;
    }
}

Midi1ControllerCatalog::Midi1ControllerCatalog()
    : enabledRpns(createStandardRpnEnabled())
    , enabledNrpns({}) {
}

void Midi1ControllerCatalog::enableAllNrpnMsbs() {
    for (int i = 0; i < 0x80; i++) {
        enabledNrpns[i * 0x80] = true;
    }
}

Midi1MachineChannel::Midi1MachineChannel()
    : noteOnStatus({})
    , noteVelocity({})
    , pafVelocity({})
    , controls({})
    , rpns({})
    , nrpns({}) {
}

int Midi1MachineChannel::getCurrentRPN() const {
    return (toUnsigned(controls[MidiCC::RPN_MSB]) << 7) + toUnsigned(controls[MidiCC::RPN_LSB]);
}

int Midi1MachineChannel::getCurrentNRPN() const {
    return (toUnsigned(controls[MidiCC::NRPN_MSB]) << 7) + toUnsigned(controls[MidiCC::NRPN_LSB]);
}

void Midi1MachineChannel::processDte(uint8_t value, bool isMsb) {
    std::array<int16_t, 128 * 128>* arr;
    int target;

    if (dteTarget == DteTarget::RPN) {
        target = getCurrentRPN();
        arr = &rpns;
    } else {
        target = getCurrentNRPN();
        arr = &nrpns;
    }

    int cur = (*arr)[target];
    if (isMsb) {
        (*arr)[target] = static_cast<int16_t>((cur & 0x007F) + ((toUnsigned(value) & 0x7F) << 7));
    } else {
        (*arr)[target] = static_cast<int16_t>((cur & 0x3F80) + (toUnsigned(value) & 0x7F));
    }
}

void Midi1MachineChannel::processDteIncrement() {
    if (dteTarget == DteTarget::RPN) {
        rpns[controls[MidiCC::RPN_MSB] * 0x80 + controls[MidiCC::RPN_LSB]]++;
    } else {
        nrpns[controls[MidiCC::NRPN_MSB] * 0x80 + controls[MidiCC::NRPN_LSB]]++;
    }
}

void Midi1MachineChannel::processDteDecrement() {
    if (dteTarget == DteTarget::RPN) {
        rpns[controls[MidiCC::RPN_MSB] * 0x80 + controls[MidiCC::RPN_LSB]]--;
    } else {
        nrpns[controls[MidiCC::NRPN_MSB] * 0x80 + controls[MidiCC::NRPN_LSB]]--;
    }
}

void Midi1Machine::processMessage(const Midi1Message& message) {
    uint8_t channel = message.getChannel();
    uint8_t statusCode = message.getStatusCode();

    switch (statusCode) {
        case MidiChannelStatus::NOTE_ON:
            channels[channel].noteVelocity[toUnsigned(message.getMsb())] = message.getLsb();
            channels[channel].noteOnStatus[toUnsigned(message.getMsb())] = true;
            break;

        case MidiChannelStatus::NOTE_OFF:
            channels[channel].noteVelocity[toUnsigned(message.getMsb())] = message.getLsb();
            channels[channel].noteOnStatus[toUnsigned(message.getMsb())] = false;
            break;

        case MidiChannelStatus::PAF:
            channels[channel].pafVelocity[toUnsigned(message.getMsb())] = message.getLsb();
            break;

        case MidiChannelStatus::CC: {
            uint8_t ccNumber = message.getMsb();
            uint8_t ccValue = message.getLsb();

            switch (ccNumber) {
                case MidiCC::NRPN_MSB:
                case MidiCC::NRPN_LSB:
                    channels[channel].dteTarget = DteTarget::NRPN;
                    break;
                case MidiCC::RPN_MSB:
                case MidiCC::RPN_LSB:
                    channels[channel].dteTarget = DteTarget::RPN;
                    break;
                case MidiCC::DTE_MSB:
                    channels[channel].processDte(ccValue, true);
                    break;
                case MidiCC::DTE_LSB:
                    channels[channel].processDte(ccValue, false);
                    break;
                case MidiCC::DTE_INCREMENT:
                    channels[channel].processDteIncrement();
                    break;
                case MidiCC::DTE_DECREMENT:
                    channels[channel].processDteDecrement();
                    break;
            }

            channels[channel].controls[toUnsigned(ccNumber)] = ccValue;

            switch (ccNumber) {
                case MidiCC::OMNI_MODE_OFF:
                    channels[channel].omniMode = false;
                    break;
                case MidiCC::OMNI_MODE_ON:
                    channels[channel].omniMode = true;
                    break;
                case MidiCC::MONO_MODE_ON:
                    channels[channel].monoPolyMode = false;
                    break;
                case MidiCC::POLY_MODE_ON:
                    channels[channel].monoPolyMode = true;
                    break;
            }
            break;
        }

        case MidiChannelStatus::PROGRAM:
            channels[channel].program = message.getMsb();
            break;

        case MidiChannelStatus::CAF:
            channels[channel].caf = message.getMsb();
            break;

        case MidiChannelStatus::PITCH_BEND:
            channels[channel].pitchbend = static_cast<int16_t>(
                (toUnsigned(message.getMsb()) << 7) + toUnsigned(message.getLsb())
            );
            break;
    }

    for (auto& listener : messageListeners) {
        listener(message);
    }
}

}
