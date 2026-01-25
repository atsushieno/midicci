#pragma once

#include <cstdint>

namespace umppi {

enum class MidiTransportProtocol : uint8_t {
    MIDI1 = 1,
    UMP = 2
};

namespace MidiMessageType {
    constexpr uint8_t UTILITY = 0;
    constexpr uint8_t SYSTEM = 1;
    constexpr uint8_t MIDI1 = 2;
    constexpr uint8_t SYSEX7 = 3;
    constexpr uint8_t MIDI2 = 4;
    constexpr uint8_t SYSEX8_MDS = 5;
    constexpr uint8_t FLEX_DATA = 0xD;
    constexpr uint8_t UMP_STREAM = 0xF;
}

namespace MidiUtilityStatus {
    constexpr uint16_t NOP = 0x0000;
    constexpr uint16_t JR_CLOCK = 0x0010;
    constexpr uint16_t JR_TIMESTAMP = 0x0020;
    constexpr uint16_t DCTPQ = 0x0030;
    constexpr uint16_t DELTA_CLOCKSTAMP = 0x0040;
}

namespace MidiSystemStatus {
    constexpr uint8_t MIDI_TIME_CODE = 0xF1;
    constexpr uint8_t SONG_POSITION = 0xF2;
    constexpr uint8_t SONG_SELECT = 0xF3;
    constexpr uint8_t TUNE_REQUEST = 0xF6;
    constexpr uint8_t TIMING_CLOCK = 0xF8;
    constexpr uint8_t START = 0xFA;
    constexpr uint8_t CONTINUE = 0xFB;
    constexpr uint8_t STOP = 0xFC;
    constexpr uint8_t ACTIVE_SENSING = 0xFE;
    constexpr uint8_t RESET = 0xFF;
}

namespace MidiChannelStatus {
    constexpr uint8_t NOTE_OFF = 0x80;
    constexpr uint8_t NOTE_ON = 0x90;
    constexpr uint8_t PAF = 0xA0;
    constexpr uint8_t CC = 0xB0;
    constexpr uint8_t PROGRAM = 0xC0;
    constexpr uint8_t CAF = 0xD0;
    constexpr uint8_t PITCH_BEND = 0xE0;

    constexpr uint8_t PER_NOTE_RCC = 0x00;
    constexpr uint8_t PER_NOTE_ACC = 0x10;
    constexpr uint8_t RPN = 0x20;
    constexpr uint8_t NRPN = 0x30;
    constexpr uint8_t RELATIVE_RPN = 0x40;
    constexpr uint8_t RELATIVE_NRPN = 0x50;
    constexpr uint8_t PER_NOTE_PITCH_BEND = 0x60;
    constexpr uint8_t PER_NOTE_MANAGEMENT = 0xF0;
}

namespace Midi1Status {
    constexpr uint8_t SYSEX = 0xF0;
    constexpr uint8_t SYSEX_END = 0xF7;
    constexpr uint8_t META = 0xFF;
}

namespace Midi2BinaryChunkStatus {
    constexpr uint8_t COMPLETE_PACKET = 0;
    constexpr uint8_t START = 0x10;
    constexpr uint8_t CONTINUE = 0x20;
    constexpr uint8_t END = 0x30;
    constexpr uint8_t MDS_HEADER = 0x80;
    constexpr uint8_t MDS_PAYLOAD = 0x90;
}

namespace Midi2BinaryChunkFormat {
    constexpr uint8_t COMPLETE_PACKET = 0;
    constexpr uint8_t START = 1;
    constexpr uint8_t CONTINUE = 2;
    constexpr uint8_t END = 3;
}

namespace MidiMetaType {
    constexpr uint8_t SEQUENCE_NUMBER = 0;
    constexpr uint8_t TEXT = 1;
    constexpr uint8_t COPYRIGHT = 2;
    constexpr uint8_t TRACK_NAME = 3;
    constexpr uint8_t INSTRUMENT_NAME = 4;
    constexpr uint8_t LYRIC = 5;
    constexpr uint8_t MARKER = 6;
    constexpr uint8_t CUE_POINT = 7;
    constexpr uint8_t CHANNEL_PREFIX = 0x20;
    constexpr uint8_t END_OF_TRACK = 0x2F;
    constexpr uint8_t TEMPO = 0x51;
    constexpr uint8_t SMPTE_OFFSET = 0x54;
    constexpr uint8_t TIME_SIGNATURE = 0x58;
    constexpr uint8_t KEY_SIGNATURE = 0x59;
    constexpr uint8_t SEQUENCER_SPECIFIC = 0x7F;
}

namespace MidiCC {
    constexpr uint8_t BANK_SELECT = 0;
    constexpr uint8_t MODULATION = 1;
    constexpr uint8_t BREATH = 2;
    constexpr uint8_t FOOT = 4;
    constexpr uint8_t PORTAMENTO_TIME = 5;
    constexpr uint8_t DTE_MSB = 6;
    constexpr uint8_t VOLUME = 7;
    constexpr uint8_t BALANCE = 8;
    constexpr uint8_t PAN = 10;
    constexpr uint8_t EXPRESSION = 11;
    constexpr uint8_t EFFECT_CONTROL_1 = 12;
    constexpr uint8_t EFFECT_CONTROL_2 = 13;
    constexpr uint8_t GENERAL_1 = 16;
    constexpr uint8_t GENERAL_2 = 17;
    constexpr uint8_t GENERAL_3 = 18;
    constexpr uint8_t GENERAL_4 = 19;
    constexpr uint8_t BANK_SELECT_LSB = 32;
    constexpr uint8_t DTE_LSB = 38;
    constexpr uint8_t HOLD = 64;
    constexpr uint8_t PORTAMENTO = 65;
    constexpr uint8_t SOSTENUTO = 66;
    constexpr uint8_t SOFT_PEDAL = 67;
    constexpr uint8_t LEGATO = 68;
    constexpr uint8_t HOLD_2 = 69;
    constexpr uint8_t SOUND_CONTROLLER_1 = 70;
    constexpr uint8_t SOUND_CONTROLLER_2 = 71;
    constexpr uint8_t SOUND_CONTROLLER_3 = 72;
    constexpr uint8_t SOUND_CONTROLLER_4 = 73;
    constexpr uint8_t SOUND_CONTROLLER_5 = 74;
    constexpr uint8_t SOUND_CONTROLLER_6 = 75;
    constexpr uint8_t SOUND_CONTROLLER_7 = 76;
    constexpr uint8_t SOUND_CONTROLLER_8 = 77;
    constexpr uint8_t SOUND_CONTROLLER_9 = 78;
    constexpr uint8_t SOUND_CONTROLLER_10 = 79;
    constexpr uint8_t GENERAL_5 = 80;
    constexpr uint8_t GENERAL_6 = 81;
    constexpr uint8_t GENERAL_7 = 82;
    constexpr uint8_t GENERAL_8 = 83;
    constexpr uint8_t PORTAMENTO_CONTROL = 84;
    constexpr uint8_t RSD = 91;
    constexpr uint8_t EFFECT_1 = 91;
    constexpr uint8_t EFFECT_2 = 92;
    constexpr uint8_t EFFECT_3 = 93;
    constexpr uint8_t EFFECT_4 = 94;
    constexpr uint8_t EFFECT_5 = 95;
    constexpr uint8_t DTE_INCREMENT = 96;
    constexpr uint8_t DTE_DECREMENT = 97;
    constexpr uint8_t NRPN_LSB = 98;
    constexpr uint8_t NRPN_MSB = 99;
    constexpr uint8_t RPN_LSB = 100;
    constexpr uint8_t RPN_MSB = 101;
    constexpr uint8_t ALL_SOUND_OFF = 120;
    constexpr uint8_t RESET_ALL_CONTROLLERS = 121;
    constexpr uint8_t LOCAL_CONTROL = 122;
    constexpr uint8_t ALL_NOTES_OFF = 123;
    constexpr uint8_t OMNI_MODE_OFF = 124;
    constexpr uint8_t OMNI_MODE_ON = 125;
    constexpr uint8_t MONO_MODE_ON = 126;
    constexpr uint8_t POLY_MODE_ON = 127;
}

namespace MidiRpn {
    constexpr uint16_t PITCH_BEND_SENSITIVITY = 0;
    constexpr uint16_t FINE_TUNING = 1;
    constexpr uint16_t COARSE_TUNING = 2;
    constexpr uint16_t TUNING_PROGRAM = 3;
    constexpr uint16_t TUNING_BANK_SELECT = 4;
    constexpr uint16_t MODULATION_DEPTH = 5;
    constexpr uint16_t NULL_FUNCTION = 0x7F7F;
}

namespace UmpStreamStatus {
    constexpr uint16_t ENDPOINT_DISCOVERY = 0x00;
    constexpr uint16_t ENDPOINT_INFO = 0x01;
    constexpr uint16_t DEVICE_IDENTITY = 0x02;
    constexpr uint16_t ENDPOINT_NAME = 0x03;
    constexpr uint16_t PRODUCT_INSTANCE_ID = 0x04;
    constexpr uint16_t STREAM_CONFIG_REQUEST = 0x05;
    constexpr uint16_t STREAM_CONFIG_NOTIFICATION = 0x06;
    constexpr uint16_t FUNCTION_BLOCK_DISCOVERY = 0x10;
    constexpr uint16_t FUNCTION_BLOCK_INFO = 0x11;
    constexpr uint16_t FUNCTION_BLOCK_NAME = 0x12;
    constexpr uint16_t START_OF_CLIP = 0x20;
    constexpr uint16_t END_OF_CLIP = 0x21;
}

namespace FlexDataStatus {
    constexpr uint8_t TEMPO = 0;
    constexpr uint8_t TIME_SIGNATURE = 1;
    constexpr uint8_t METRONOME = 2;
    constexpr uint8_t KEY_SIGNATURE = 5;
    constexpr uint8_t CHORD_NAME = 6;
}

namespace MidiProgramChangeOptions {
    constexpr uint8_t NONE = 0;
    constexpr uint8_t BANK_VALID = 1;
}

namespace MidiNoteAttributeType {
    constexpr uint8_t NONE = 0;
    constexpr uint8_t Pitch7_9 = 3;
}

} // namespace umppi
