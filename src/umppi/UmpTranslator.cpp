#include <umppi/details/UmpTranslator.hpp>
#include <umppi/details/UmpFactory.hpp>
#include <algorithm>

namespace {

enum class SmfMetaProcessResult {
    HANDLED,
    SKIPPED,
    INVALID
};

bool readVariableLengthQuantity(const std::vector<uint8_t>& data, size_t& pos, uint32_t& value) {
    value = 0;
    int bytesConsumed = 0;
    while (true) {
        if (pos >= data.size() || bytesConsumed == 4) {
            return false;
        }
        uint8_t byte = data[pos++];
        value = (value << 7) | static_cast<uint32_t>(byte & 0x7F);
        ++bytesConsumed;
        if ((byte & 0x80) == 0) {
            break;
        }
    }
    return true;
}

uint8_t mapMajorKeyTonic(int sharpsOrFlats) {
    static constexpr uint8_t tonics[15] = {
        umppi::TonicNoteField::C, // -7
        umppi::TonicNoteField::G, // -6
        umppi::TonicNoteField::D, // -5
        umppi::TonicNoteField::A, // -4
        umppi::TonicNoteField::E, // -3
        umppi::TonicNoteField::B, // -2
        umppi::TonicNoteField::F, // -1
        umppi::TonicNoteField::C, // 0
        umppi::TonicNoteField::G, // 1
        umppi::TonicNoteField::D, // 2
        umppi::TonicNoteField::A, // 3
        umppi::TonicNoteField::E, // 4
        umppi::TonicNoteField::B, // 5
        umppi::TonicNoteField::F, // 6 (F#)
        umppi::TonicNoteField::C  // 7 (C#)
    };
    int index = sharpsOrFlats + 7;
    constexpr int tonicsCount = static_cast<int>(sizeof(tonics) / sizeof(tonics[0]));
    if (index < 0 || index >= tonicsCount) {
        return umppi::TonicNoteField::UNKNOWN;
    }
    return tonics[index];
}

uint8_t mapMinorKeyTonic(int sharpsOrFlats) {
    static constexpr uint8_t tonics[15] = {
        umppi::TonicNoteField::A, // -7 (Ab minor)
        umppi::TonicNoteField::E, // -6
        umppi::TonicNoteField::B, // -5
        umppi::TonicNoteField::F, // -4
        umppi::TonicNoteField::C, // -3
        umppi::TonicNoteField::G, // -2
        umppi::TonicNoteField::D, // -1
        umppi::TonicNoteField::A, // 0
        umppi::TonicNoteField::E, // 1
        umppi::TonicNoteField::B, // 2
        umppi::TonicNoteField::F, // 3 (F# minor)
        umppi::TonicNoteField::C, // 4 (C# minor)
        umppi::TonicNoteField::G, // 5 (G# minor)
        umppi::TonicNoteField::D, // 6 (D# minor)
        umppi::TonicNoteField::A  // 7 (A# minor)
    };
    int index = sharpsOrFlats + 7;
    constexpr int tonicsCount = static_cast<int>(sizeof(tonics) / sizeof(tonics[0]));
    if (index < 0 || index >= tonicsCount) {
        return umppi::TonicNoteField::UNKNOWN;
    }
    return tonics[index];
}

uint8_t resolveKeySignatureTonic(int sharpsOrFlats, bool isMinor) {
    return isMinor ? mapMinorKeyTonic(sharpsOrFlats) : mapMajorKeyTonic(sharpsOrFlats);
}

SmfMetaProcessResult translateMetaToFlexData(umppi::Midi1ToUmpTranslatorContext& context,
                                             uint8_t metaType,
                                             const std::vector<uint8_t>& data) {
    using namespace umppi;
    switch (metaType) {
        case MidiMetaType::TEMPO:
            if (data.size() != 3) {
                return SmfMetaProcessResult::INVALID;
            }
            {
                uint32_t tempoMicroseconds = (static_cast<uint32_t>(data[0]) << 16) |
                                             (static_cast<uint32_t>(data[1]) << 8) |
                                             data[2];
                context.tempo = static_cast<int>(tempoMicroseconds);
                uint32_t tempo10Nanoseconds = tempoMicroseconds * 100;
                context.output.emplace_back(UmpFactory::tempo(context.group, 0, tempo10Nanoseconds));
            }
            return SmfMetaProcessResult::HANDLED;

        case MidiMetaType::TIME_SIGNATURE:
            if (data.size() < 4) {
                return SmfMetaProcessResult::INVALID;
            }
            {
                uint8_t numerator = data[0];
                uint8_t denominatorShift = data[1];
                uint32_t denominatorValue = (denominatorShift < 8) ? (1u << denominatorShift) : 0;
                uint8_t numberOf32Notes = data[3];
                context.output.emplace_back(UmpFactory::timeSignatureDirect(
                    context.group, 0, numerator, static_cast<uint8_t>(denominatorValue), numberOf32Notes));
            }
            return SmfMetaProcessResult::HANDLED;

        case MidiMetaType::KEY_SIGNATURE:
            if (data.size() < 2) {
                return SmfMetaProcessResult::INVALID;
            }
            {
                int8_t sharpsOrFlats = static_cast<int8_t>(data[0]);
                bool isMinor = data[1] != 0;
                uint8_t tonic = resolveKeySignatureTonic(sharpsOrFlats, isMinor);
                context.output.emplace_back(
                    UmpFactory::keySignature(context.group, FlexDataAddress::GROUP, 0, sharpsOrFlats, tonic));
            }
            return SmfMetaProcessResult::HANDLED;

        case MidiMetaType::TEXT: {
            auto umps = UmpFactory::metadataText(context.group, FlexDataAddress::GROUP, 0,
                                                 MetadataTextStatus::UNKNOWN, data);
            context.output.insert(context.output.end(), umps.begin(), umps.end());
            return SmfMetaProcessResult::HANDLED;
        }

        case MidiMetaType::COPYRIGHT: {
            auto umps = UmpFactory::metadataText(context.group, FlexDataAddress::GROUP, 0,
                                                 MetadataTextStatus::COPYRIGHT, data);
            context.output.insert(context.output.end(), umps.begin(), umps.end());
            return SmfMetaProcessResult::HANDLED;
        }

        case MidiMetaType::TRACK_NAME: {
            auto umps = UmpFactory::metadataText(context.group, FlexDataAddress::GROUP, 0,
                                                 MetadataTextStatus::MIDI_CLIP_NAME, data);
            context.output.insert(context.output.end(), umps.begin(), umps.end());
            return SmfMetaProcessResult::HANDLED;
        }

        case MidiMetaType::INSTRUMENT_NAME: {
            auto umps = UmpFactory::metadataText(context.group, FlexDataAddress::GROUP, 0,
                                                 MetadataTextStatus::PRIMARY_PERFORMER, data);
            context.output.insert(context.output.end(), umps.begin(), umps.end());
            return SmfMetaProcessResult::HANDLED;
        }

        case MidiMetaType::LYRIC: {
            auto umps = UmpFactory::performanceText(context.group, FlexDataAddress::GROUP, 0,
                                                    PerformanceTextStatus::LYRICS, data);
            context.output.insert(context.output.end(), umps.begin(), umps.end());
            return SmfMetaProcessResult::HANDLED;
        }

        case MidiMetaType::MARKER:
        case MidiMetaType::CUE_POINT: {
            auto umps = UmpFactory::metadataText(context.group, FlexDataAddress::GROUP, 0,
                                                 MetadataTextStatus::UNKNOWN, data);
            context.output.insert(context.output.end(), umps.begin(), umps.end());
            return SmfMetaProcessResult::HANDLED;
        }

        case MidiMetaType::END_OF_TRACK:
            return SmfMetaProcessResult::SKIPPED;

        default:
            return SmfMetaProcessResult::SKIPPED;
    }
}

} // namespace

namespace umppi {


int UmpTranslator::translateUmpToMidi1Bytes(std::vector<uint8_t>& dst,
                                            const std::vector<Ump>& src,
                                            const UmpToMidi1BytesTranslatorContext& context) {
    // For now, implement a simplified version without SMF delta time support
    dst.clear();
    std::vector<uint8_t> sysex7;
    int deltaTime = 0;
    
    for (const auto& ump : src) {
        if (ump.isDeltaClockstamp()) {
            deltaTime += ump.getDeltaClockstamp();
            continue;
        }
        if (ump.isJRTimestamp()) {
            if (!context.skipDeltaTime) {
                deltaTime += ump.getJRTimestamp();
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
        if (ump.getMessageType() == MessageType::SYSEX7) {
            BinaryChunkStatus status = ump.getBinaryChunkStatus();
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
    uint8_t statusCode = ump.getStatusCode();
    size_t offset = dstOffset;
    
    auto addDeltaTimeAndStatus = [&]() {
        if (deltaTime >= 0) {
            // Simplified delta time encoding (just one byte for now)
            dst[offset++] = static_cast<uint8_t>(deltaTime & 0x7F);
        }
        dst[offset++] = ump.getStatusByte();
    };
    
    switch (ump.getMessageType()) {
        case MessageType::SYSTEM:
            addDeltaTimeAndStatus();
            midiEventSize = 1;
            switch (statusCode) {
                case 0xF1:
                case 0xF3:
                case 0xF9:
                    dst[offset++] = ump.getMidi1Msb();
                    midiEventSize = 2;
                    break;
            }
            break;
            
        case MessageType::MIDI1:
            addDeltaTimeAndStatus();
            midiEventSize = 3;
            dst[offset++] = ump.getMidi1Msb();
            switch (statusCode) {
                case MidiChannelStatus::PROGRAM:
                case MidiChannelStatus::CAF:
                    midiEventSize = 2;
                    break;
                default:
                    dst[offset++] = ump.getMidi1Lsb();
                    break;
            }
            break;
            
        case MessageType::MIDI2:
            switch (statusCode) {
                case MidiChannelStatus::RPN: {
                    midiEventSize = 12;
                    uint8_t channel = ump.getChannelInGroup();
                    uint8_t ccStatus = channel + MidiChannelStatus::CC;
                    
                    dst[offset + 0] = ccStatus;
                    dst[offset + 1] = MidiCC::RPN_MSB;
                    dst[offset + 2] = ump.getMidi2RpnMsb();
                    dst[offset + 3] = ccStatus;
                    dst[offset + 4] = MidiCC::RPN_LSB;
                    dst[offset + 5] = ump.getMidi2RpnLsb();
                    dst[offset + 6] = ccStatus;
                    dst[offset + 7] = MidiCC::DTE_MSB;
                    dst[offset + 8] = static_cast<uint8_t>((ump.getMidi2RpnData() >> 25) & 0x7F);
                    dst[offset + 9] = ccStatus;
                    dst[offset + 10] = MidiCC::DTE_LSB;
                    dst[offset + 11] = static_cast<uint8_t>((ump.getMidi2RpnData() >> 18) & 0x7F);
                    break;
                }
                
                case MidiChannelStatus::NRPN: {
                    midiEventSize = 12;
                    uint8_t channel = ump.getChannelInGroup();
                    uint8_t ccStatus = channel + MidiChannelStatus::CC;
                    
                    dst[offset + 0] = ccStatus;
                    dst[offset + 1] = MidiCC::NRPN_MSB;
                    dst[offset + 2] = ump.getMidi2NrpnMsb();
                    dst[offset + 3] = ccStatus;
                    dst[offset + 4] = MidiCC::NRPN_LSB;
                    dst[offset + 5] = ump.getMidi2NrpnLsb();
                    dst[offset + 6] = ccStatus;
                    dst[offset + 7] = MidiCC::DTE_MSB;
                    dst[offset + 8] = static_cast<uint8_t>((ump.getMidi2NrpnData() >> 25) & 0x7F);
                    dst[offset + 9] = ccStatus;
                    dst[offset + 10] = MidiCC::DTE_LSB;
                    dst[offset + 11] = static_cast<uint8_t>((ump.getMidi2NrpnData() >> 18) & 0x7F);
                    break;
                }
                
                case MidiChannelStatus::NOTE_OFF:
                case MidiChannelStatus::NOTE_ON:
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    dst[offset++] = ump.getMidi2Note();
                    dst[offset++] = static_cast<uint8_t>(ump.getMidi2Velocity16() / 0x200);
                    break;
                    
                case MidiChannelStatus::PAF:
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    dst[offset++] = ump.getMidi2Note();
                    dst[offset++] = static_cast<uint8_t>(ump.getMidi2PafData() / 0x2000000U);
                    break;
                    
                case MidiChannelStatus::CC:
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    dst[offset++] = ump.getMidi2CcIndex();
                    dst[offset++] = static_cast<uint8_t>(ump.getMidi2CcData() / 0x2000000U);
                    break;
                    
                case MidiChannelStatus::PROGRAM: {
                    if (ump.getMidi2ProgramOptions() & MidiProgramChangeOptions::BANK_VALID) {
                        midiEventSize = 8;
                        uint8_t channel = ump.getChannelInGroup();
                        dst[offset + 0] = channel + MidiChannelStatus::CC;
                        dst[offset + 1] = MidiCC::BANK_SELECT;
                        dst[offset + 2] = ump.getMidi2ProgramBankMsb();
                        dst[offset + 3] = channel + MidiChannelStatus::CC;
                        dst[offset + 4] = MidiCC::BANK_SELECT_LSB;
                        dst[offset + 5] = ump.getMidi2ProgramBankLsb();
                        dst[offset + 6] = channel + MidiChannelStatus::PROGRAM;
                        dst[offset + 7] = ump.getMidi2ProgramProgram();
                    } else {
                        midiEventSize = 2;
                        dst[offset + 0] = ump.getChannelInGroup() + MidiChannelStatus::PROGRAM;
                        dst[offset + 1] = ump.getMidi2ProgramProgram();
                    }
                    break;
                }
                
                case MidiChannelStatus::CAF:
                    addDeltaTimeAndStatus();
                    midiEventSize = 2;
                    dst[offset++] = static_cast<uint8_t>(ump.getMidi2CafData() / 0x2000000U);
                    break;
                    
                case MidiChannelStatus::PITCH_BEND: {
                    addDeltaTimeAndStatus();
                    midiEventSize = 3;
                    uint32_t pitchBendV1 = ump.getMidi2PitchBendData() / 0x40000U;
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
                uint8_t size = ump.getSysex7Size();
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
        if (context.isMidi1Smf) {
            uint32_t deltaTime = 0;
            int deltaBytesConsumed = 0;
            bool continueDecoding = true;
            while (continueDecoding) {
                if (context.midi1Pos >= context.midi1.size()) {
                    return UmpTranslationResult::INVALID_STATUS;
                }
                uint8_t byte = context.midi1[context.midi1Pos++];
                deltaTime = (deltaTime << 7) | static_cast<uint32_t>(byte & 0x7F);
                ++deltaBytesConsumed;
                continueDecoding = (byte & 0x80) != 0;
                if (continueDecoding && deltaBytesConsumed == 4) {
                    return UmpTranslationResult::INVALID_STATUS;
                }
            }
            if (deltaTime > 0) {
                while (deltaTime > 0xFFFFF) {
                    context.output.emplace_back(UmpFactory::deltaClockstamp(0xFFFFF));
                    deltaTime -= 0xFFFFF;
                }
                context.output.emplace_back(UmpFactory::deltaClockstamp(deltaTime));
            }
            if (context.midi1Pos >= context.midi1.size()) {
                return UmpTranslationResult::INVALID_STATUS;
            }
        }

        if (context.isMidi1Smf && context.midi1[context.midi1Pos] == Midi1Status::META) {
            if (context.midi1Pos + 2 >= context.midi1.size()) {
                return UmpTranslationResult::INVALID_STATUS;
            }
            uint8_t metaType = context.midi1[context.midi1Pos + 1];
            size_t metaPos = context.midi1Pos + 2;
            uint32_t metaLength = 0;
            if (!readVariableLengthQuantity(context.midi1, metaPos, metaLength)) {
                return UmpTranslationResult::INVALID_STATUS;
            }
            if (metaPos + metaLength > context.midi1.size()) {
                return UmpTranslationResult::INVALID_STATUS;
            }
            std::vector<uint8_t> metaData(context.midi1.begin() + metaPos,
                                          context.midi1.begin() + metaPos + metaLength);
            SmfMetaProcessResult metaResult = translateMetaToFlexData(context, metaType, metaData);
            if (metaResult == SmfMetaProcessResult::INVALID) {
                return UmpTranslationResult::INVALID_STATUS;
            }
            context.midi1Pos = metaPos + metaLength;
            continue;
        }
        
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
            
            if (context.midiProtocol == static_cast<int>(MidiTransportProtocol::MIDI1)) {
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


} // namespace midicci
