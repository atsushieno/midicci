#include "midicci/details/ump/UmpTranslator.hpp"
#include "midicci/details/ump/UmpFactory.hpp"
#include <algorithm>

namespace midicci {
namespace ump {

int UmpTranslator::translateUmpToMidi1Bytes(std::vector<uint8_t>& dst,
                                            const std::vector<Ump>& src,
                                            const UmpToMidi1BytesTranslatorContext& context) {
    // For now, implement a simplified version without SMF delta time support
    dst.clear();
    std::vector<uint8_t> sysex7;
    int deltaTime = 0;
    
    for (const auto& ump : src) {
        if (ump.is_delta_clockstamp()) {
            deltaTime += ump.get_delta_clockstamp();
            continue;
        }
        if (ump.is_jr_timestamp()) {
            if (!context.skipDeltaTime) {
                deltaTime += ump.get_jr_timestamp();
            }
            continue;
        }
        
        size_t oldSize = dst.size();
        dst.resize(dst.size() + 16); // Reserve space for the message
        
        int messageSize = translateSingleUmpToMidi1Bytes(dst, ump, oldSize, 
                                                         context.skipDeltaTime ? -1 : deltaTime, 
                                                         &sysex7);
        
        dst.resize(oldSize + messageSize); // Trim to actual size
        
        // Handle SysEx7 completion
        if (ump.get_message_type() == MessageType::SYSEX7) {
            BinaryChunkStatus status = ump.get_binary_chunk_status();
            if (status == BinaryChunkStatus::END || status == BinaryChunkStatus::COMPLETE_PACKET) {
                // Add complete SysEx message
                size_t sysexStart = dst.size();
                dst.resize(dst.size() + 2 + sysex7.size()); // F0 + data + F7
                dst[sysexStart] = 0xF0;
                std::copy(sysex7.begin(), sysex7.end(), dst.begin() + sysexStart + 1);
                dst[sysexStart + 1 + sysex7.size()] = 0xF7;
                sysex7.clear();
            }
        }
        
        deltaTime = 0;
    }
    
    return sysex7.empty() ? UmpTranslationResult::OK : UmpTranslationResult::INCOMPLETE_SYSEX7;
}

int UmpTranslator::translateSingleUmpToMidi1Bytes(std::vector<uint8_t>& dst,
                                                  const Ump& ump,
                                                  size_t dstOffset,
                                                  int deltaTime,
                                                  std::vector<uint8_t>* sysex) {
    int midiEventSize = 0;
    uint8_t statusCode = ump.get_status_code();
    size_t offset = dstOffset;
    
    auto addDeltaTimeAndStatus = [&]() {
        if (deltaTime >= 0) {
            // Simplified delta time encoding (just one byte for now)
            dst[offset++] = static_cast<uint8_t>(deltaTime & 0x7F);
        }
        dst[offset++] = ump.get_status_byte();
    };
    
    switch (ump.get_message_type()) {
        case MessageType::SYSTEM:
            addDeltaTimeAndStatus();
            midiEventSize = 1;
            switch (statusCode) {
                case 0xF1:
                case 0xF3:
                case 0xF9:
                    dst[offset++] = ump.get_midi1_msb();
                    midiEventSize = 2;
                    break;
            }
            break;
            
        case MessageType::MIDI1:
            addDeltaTimeAndStatus();
            midiEventSize = 3;
            dst[offset++] = ump.get_midi1_msb();
            switch (statusCode) {
                case MidiChannelStatus::PROGRAM:
                case MidiChannelStatus::CAF:
                    midiEventSize = 2;
                    break;
                default:
                    dst[offset++] = ump.get_midi1_lsb();
                    break;
            }
            break;
            
        case MessageType::MIDI2:
            switch (statusCode) {
                case MidiChannelStatus::RPN: {
                    midiEventSize = 12;
                    uint8_t channel = ump.get_channel_in_group();
                    uint8_t ccStatus = channel + MidiChannelStatus::CC;
                    
                    dst[offset + 0] = ccStatus;
                    dst[offset + 1] = MidiCC::RPN_MSB;
                    dst[offset + 2] = ump.get_midi2_rpn_msb();
                    dst[offset + 3] = ccStatus;
                    dst[offset + 4] = MidiCC::RPN_LSB;
                    dst[offset + 5] = ump.get_midi2_rpn_lsb();
                    dst[offset + 6] = ccStatus;
                    dst[offset + 7] = MidiCC::DTE_MSB;
                    dst[offset + 8] = static_cast<uint8_t>((ump.get_midi2_rpn_data() >> 25) & 0x7F);
                    dst[offset + 9] = ccStatus;
                    dst[offset + 10] = MidiCC::DTE_LSB;
                    dst[offset + 11] = static_cast<uint8_t>((ump.get_midi2_rpn_data() >> 18) & 0x7F);
                    break;
                }
                
                case MidiChannelStatus::NRPN: {
                    midiEventSize = 12;
                    uint8_t channel = ump.get_channel_in_group();
                    uint8_t ccStatus = channel + MidiChannelStatus::CC;
                    
                    dst[offset + 0] = ccStatus;
                    dst[offset + 1] = MidiCC::NRPN_MSB;
                    dst[offset + 2] = ump.get_midi2_nrpn_msb();
                    dst[offset + 3] = ccStatus;
                    dst[offset + 4] = MidiCC::NRPN_LSB;
                    dst[offset + 5] = ump.get_midi2_nrpn_lsb();
                    dst[offset + 6] = ccStatus;
                    dst[offset + 7] = MidiCC::DTE_MSB;
                    dst[offset + 8] = static_cast<uint8_t>((ump.get_midi2_nrpn_data() >> 25) & 0x7F);
                    dst[offset + 9] = ccStatus;
                    dst[offset + 10] = MidiCC::DTE_LSB;
                    dst[offset + 11] = static_cast<uint8_t>((ump.get_midi2_nrpn_data() >> 18) & 0x7F);
                    break;
                }
                
                case MidiChannelStatus::NOTE_OFF:
                case MidiChannelStatus::NOTE_ON:
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    dst[offset++] = ump.get_midi2_note();
                    dst[offset++] = static_cast<uint8_t>(ump.get_midi2_velocity16() / 0x200);
                    break;
                    
                case MidiChannelStatus::PAF:
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    dst[offset++] = ump.get_midi2_note();
                    dst[offset++] = static_cast<uint8_t>(ump.get_midi2_paf_data() / 0x2000000U);
                    break;
                    
                case MidiChannelStatus::CC:
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    dst[offset++] = ump.get_midi2_cc_index();
                    dst[offset++] = static_cast<uint8_t>(ump.get_midi2_cc_data() / 0x2000000U);
                    break;
                    
                case MidiChannelStatus::PROGRAM: {
                    if (ump.get_midi2_program_options() & MidiProgramChangeOptions::BANK_VALID) {
                        midiEventSize = 8;
                        uint8_t channel = ump.get_channel_in_group();
                        dst[offset + 0] = channel + MidiChannelStatus::CC;
                        dst[offset + 1] = MidiCC::BANK_SELECT;
                        dst[offset + 2] = ump.get_midi2_program_bank_msb();
                        dst[offset + 3] = channel + MidiChannelStatus::CC;
                        dst[offset + 4] = MidiCC::BANK_SELECT_LSB;
                        dst[offset + 5] = ump.get_midi2_program_bank_lsb();
                        dst[offset + 6] = channel + MidiChannelStatus::PROGRAM;
                        dst[offset + 7] = ump.get_midi2_program_program();
                    } else {
                        midiEventSize = 2;
                        dst[offset + 0] = ump.get_channel_in_group() + MidiChannelStatus::PROGRAM;
                        dst[offset + 1] = ump.get_midi2_program_program();
                    }
                    break;
                }
                
                case MidiChannelStatus::CAF:
                    addDeltaTimeAndStatus();
                    midiEventSize = 2;
                    dst[offset++] = static_cast<uint8_t>(ump.get_midi2_caf_data() / 0x2000000U);
                    break;
                    
                case MidiChannelStatus::PITCH_BEND: {
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    uint32_t pitchBendV1 = ump.get_midi2_pitch_bend_data() / 0x40000U;
                    // Note: MIDI1 pitch bend is little endian
                    dst[offset++] = static_cast<uint8_t>(pitchBendV1 & 0x7F);
                    dst[offset++] = static_cast<uint8_t>((pitchBendV1 >> 7) & 0x7F);
                    break;
                }
                
                default:
                    // Skip unsupported status bytes
                    midiEventSize = 0;
                    break;
            }
            break;
            
        case MessageType::SYSEX7:
            midiEventSize = 0;
            if (sysex) {
                // Extract SysEx data from UMP and add to accumulator
                uint8_t size = ump.get_sysex7_size();
                for (int i = 0; i < size && i < 6; ++i) {
                    uint8_t dataByte;
                    if (i == 0) dataByte = (ump.int1 >> 8) & 0x7F;
                    else if (i == 1) dataByte = ump.int1 & 0x7F;
                    else dataByte = (ump.int2 >> (24 - (i - 2) * 8)) & 0x7F;
                    sysex->push_back(dataByte);
                }
            }
            break;
            
        case MessageType::SYSEX8_MDS:
            // Cannot be translated in Default Translation
            midiEventSize = 0;
            break;
            
        default:
            // Ignore other message types
            midiEventSize = 0;
            break;
    }
    
    return midiEventSize;
}

uint64_t UmpTranslator::convertMidi1DteToUmp(Midi1ToUmpTranslatorContext& context, int channel) {
    bool isRpn = (context.rpnState & 0x8080) == 0;
    uint8_t msb = static_cast<uint8_t>((isRpn ? context.rpnState : context.nrpnState) >> 8);
    uint8_t lsb = static_cast<uint8_t>((isRpn ? context.rpnState : context.nrpnState) & 0xFF);
    uint32_t data = ((context.dteState >> 8) << 25) + ((context.dteState & 0x7F) << 18);
    
    // Reset states
    context.rpnState = 0x8080;
    context.nrpnState = 0x8080;
    context.dteState = 0x8080;
    
    return isRpn ? UmpFactory::midi2RPN(context.group, channel, msb, lsb, data)
                 : UmpFactory::midi2NRPN(context.group, channel, msb, lsb, data);
}

int UmpTranslator::getMidi1MessageSize(uint8_t statusByte) {
    uint8_t status = statusByte & 0xF0;
    switch (status) {
        case 0xC0: // Program Change
        case 0xD0: // Channel Aftertouch
            return 2;
        case 0x80: // Note Off
        case 0x90: // Note On
        case 0xA0: // Polyphonic Aftertouch
        case 0xB0: // Control Change
        case 0xE0: // Pitch Bend
            return 3;
        case 0xF0: // System messages
            switch (statusByte) {
                case 0xF1: // MIDI Time Code
                case 0xF3: // Song Select
                    return 2;
                case 0xF2: // Song Position
                    return 3;
                default:
                    return 1;
            }
        default:
            return 1;
    }
}

int UmpTranslator::translateMidi1BytesToUmp(Midi1ToUmpTranslatorContext& context) {
    while (context.midi1Pos < context.midi1.size()) {
        if (context.midi1[context.midi1Pos] == 0xF0) {
            // SysEx handling
            size_t f7Pos = context.midi1Pos + 1;
            while (f7Pos < context.midi1.size() && context.midi1[f7Pos] != 0xF7) {
                f7Pos++;
            }
            if (f7Pos >= context.midi1.size()) {
                return UmpTranslationResult::INVALID_SYSEX;
            }
            
            size_t sysexSize = f7Pos - context.midi1Pos - 1; // Exclude F0 and F7
            std::vector<uint8_t> sysexData(context.midi1.begin() + context.midi1Pos + 1,
                                           context.midi1.begin() + f7Pos);
            
            std::vector<Ump> sysexUmps = UmpFactory::sysex7(context.group, sysexData);
            context.output.insert(context.output.end(), sysexUmps.begin(), sysexUmps.end());
            
            context.midi1Pos = f7Pos + 1; // Skip past F7
        } else {
            // Fixed-size message
            int len = getMidi1MessageSize(context.midi1[context.midi1Pos]);
            if (context.midi1Pos + len > context.midi1.size()) {
                return UmpTranslationResult::INVALID_STATUS;
            }
            
            uint8_t byte2 = (len > 1) ? context.midi1[context.midi1Pos + 1] : 0;
            uint8_t byte3 = (len > 2) ? context.midi1[context.midi1Pos + 2] : 0;
            uint8_t channel = context.midi1[context.midi1Pos] & 0xF;
            
            if (context.midiProtocol == MidiTransportProtocol::MIDI1) {
                // Generate MIDI1 UMP
                uint32_t ump = UmpFactory::midi1Message(context.group,
                                                         context.midi1[context.midi1Pos] & 0xF0,
                                                         channel, byte2, byte3);
                context.output.emplace_back(ump);
                context.midi1Pos += len;
            } else {
                // Generate MIDI2 UMP
                uint64_t m2 = 0;
                const uint8_t NO_ATTRIBUTE_TYPE = 0;
                const uint16_t NO_ATTRIBUTE_DATA = 0;
                bool skipEmitUmp = false;
                
                switch (context.midi1[context.midi1Pos] & 0xF0) {
                    case MidiChannelStatus::NOTE_OFF:
                        m2 = UmpFactory::midi2NoteOff(context.group, channel, byte2, NO_ATTRIBUTE_TYPE,
                                                       static_cast<uint16_t>(byte3) << 9, NO_ATTRIBUTE_DATA);
                        break;
                        
                    case MidiChannelStatus::NOTE_ON:
                        m2 = UmpFactory::midi2NoteOn(context.group, channel, byte2, NO_ATTRIBUTE_TYPE,
                                                      static_cast<uint16_t>(byte3) << 9, NO_ATTRIBUTE_DATA);
                        break;
                        
                    case MidiChannelStatus::PAF:
                        m2 = UmpFactory::midi2PAf(context.group, channel, byte2,
                                                   static_cast<uint32_t>(byte3) << 25);
                        break;
                        
                    case MidiChannelStatus::CC:
                        switch (byte2) {
                            case MidiCC::RPN_MSB:
                                context.rpnState = (context.rpnState & 0xFF) | (static_cast<uint16_t>(byte3) << 8);
                                skipEmitUmp = true;
                                break;
                            case MidiCC::RPN_LSB:
                                context.rpnState = (context.rpnState & 0xFF00) | byte3;
                                skipEmitUmp = true;
                                break;
                            case MidiCC::NRPN_MSB:
                                context.nrpnState = (context.nrpnState & 0xFF) | (static_cast<uint16_t>(byte3) << 8);
                                skipEmitUmp = true;
                                break;
                            case MidiCC::NRPN_LSB:
                                context.nrpnState = (context.nrpnState & 0xFF00) | byte3;
                                skipEmitUmp = true;
                                break;
                            case MidiCC::DTE_MSB:
                                context.dteState = (context.dteState & 0xFF) | (static_cast<uint16_t>(byte3) << 8);
                                if (context.allowReorderedDTE && (context.dteState & 0x8080) == 0) {
                                    m2 = convertMidi1DteToUmp(context, channel);
                                } else {
                                    skipEmitUmp = true;
                                }
                                break;
                            case MidiCC::DTE_LSB:
                                context.dteState = (context.dteState & 0xFF00) | byte3;
                                if ((context.dteState & 0x8000) != 0 && !context.allowReorderedDTE) {
                                    return UmpTranslationResult::INVALID_DTE_SEQUENCE;
                                }
                                if ((context.rpnState & 0x8080) != 0 && (context.nrpnState & 0x8080) != 0) {
                                    return UmpTranslationResult::INVALID_DTE_SEQUENCE;
                                }
                                m2 = convertMidi1DteToUmp(context, channel);
                                break;
                            case MidiCC::BANK_SELECT:
                                context.bankState = (context.bankState & 0xFF) | (static_cast<uint16_t>(byte3) << 8);
                                skipEmitUmp = true;
                                break;
                            case MidiCC::BANK_SELECT_LSB:
                                context.bankState = (context.bankState & 0xFF00) | byte3;
                                skipEmitUmp = true;
                                break;
                            default:
                                m2 = UmpFactory::midi2CC(context.group, channel, byte2,
                                                          static_cast<uint32_t>(byte3) << 25);
                                break;
                        }
                        break;
                        
                    case MidiChannelStatus::PROGRAM: {
                        bool bankMsbValid = (context.bankState & 0x8000) == 0;
                        bool bankLsbValid = (context.bankState & 0x80) == 0;
                        bool bankValid = bankMsbValid || bankLsbValid;
                        
                        m2 = UmpFactory::midi2Program(context.group, channel,
                                                       bankValid ? MidiProgramChangeOptions::BANK_VALID : MidiProgramChangeOptions::NONE,
                                                       byte2,
                                                       bankMsbValid ? static_cast<uint8_t>(context.bankState >> 8) : 0,
                                                       bankLsbValid ? static_cast<uint8_t>(context.bankState & 0x7F) : 0);
                        context.bankState = 0x8080;
                        break;
                    }
                    
                    case MidiChannelStatus::CAF:
                        m2 = UmpFactory::midi2CAf(context.group, channel, static_cast<uint32_t>(byte2) << 25);
                        break;
                        
                    case MidiChannelStatus::PITCH_BEND:
                        // MIDI1 pitch bend is little endian
                        m2 = UmpFactory::midi2PitchBendDirect(context.group, channel,
                                                               static_cast<uint32_t>(((byte3 << 7) + byte2) << 18));
                        break;
                        
                    default:
                        return UmpTranslationResult::INVALID_STATUS;
                }
                
                if (!skipEmitUmp) {
                    context.output.emplace_back(m2);
                }
                context.midi1Pos += len;
            }
        }
    }
    
    // Check for incomplete DTE sequences
    if (context.rpnState != 0x8080 || context.nrpnState != 0x8080 || context.dteState != 0x8080) {
        return UmpTranslationResult::INVALID_DTE_SEQUENCE;
    }
    
    return UmpTranslationResult::OK;
}

} // namespace ump
} // namespace midicci