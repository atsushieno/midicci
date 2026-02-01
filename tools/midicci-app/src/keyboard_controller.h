#pragma once

#include <libremidi/libremidi.hpp>
#include <libremidi/ump.hpp>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <optional>
#include "midi_ci_manager.h"
#include "message_logger.h"

class KeyboardController {
public:
    struct IncomingControlValue {
        std::string ctrlType;
        std::vector<uint8_t> ctrlIndex;
        uint32_t value = 0;
        uint8_t group = 0;
        uint8_t channel = 0;
        std::optional<uint8_t> note;
    };

    KeyboardController(midicci::keyboard::MessageLogger* logger = nullptr);
    ~KeyboardController();
    
    bool resetMidiConnections();
    void noteOn(int note, int velocity);
    void noteOff(int note);
    void allNotesOff();
    
    // Device enumeration
    std::vector<std::pair<std::string, std::string>> getInputDevices();
    std::vector<std::pair<std::string, std::string>> getOutputDevices();
    
    // Device selection
    bool selectInputDevice(const std::string& deviceId);
    bool selectOutputDevice(const std::string& deviceId);
    
    void refreshDevices();
    
    // MIDI-CI functionality
    void sendMidiCIDiscovery();
    std::vector<std::string> getMidiCIDevices();
    std::vector<MidiCIDeviceInfo> getMidiCIDeviceDetails();
    MidiCIDeviceInfo* getMidiCIDeviceByMuid(uint32_t muid);
    bool isMidiCIInitialized() const;
    uint32_t getMidiCIMuid() const;
    std::string getMidiCIDeviceName() const;
    void setMidiCIDevicesChangedCallback(std::function<void()> callback);
    
    // MIDI-CI Property functionality - simplified API using PropertyClientFacade
    std::optional<std::vector<midicci::commonproperties::MidiCIControl>> getAllCtrlList(uint32_t muid);
    std::optional<std::vector<midicci::commonproperties::MidiCIProgram>> getProgramList(uint32_t muid);
    std::optional<std::vector<midicci::commonproperties::MidiCIControlMap>> getCtrlMapList(uint32_t muid, const std::string& ctrlMapId);
    void setMidiCIPropertiesChangedCallback(std::function<void(uint32_t, const std::string&, const std::string&)> callback);

    // Explicit property requests (user-triggered)
    void requestCtrlMapList(uint32_t muid, const std::string& ctrlMapId);
    void requestAllCtrlList(uint32_t muid);
    void requestProgramList(uint32_t muid);
    void requestSaveState(uint32_t muid);

    // State management
    void sendState(uint32_t muid, const std::string& stateId, const std::vector<uint8_t>& data);
    void setStateSaveCallback(std::function<void(uint32_t, const std::vector<uint8_t>&)> callback);

    // MIDI control sending
    void sendControlChange(int channel, int controller, uint32_t value, int group = 0);
    void sendRPN(int channel, int msb, int lsb, uint32_t value, int group = 0);
    void sendNRPN(int channel, int msb, int lsb, uint32_t value, int group = 0);
    void sendPerNoteControlChange(int channel, int note, int controller, uint32_t value, int group = 0);
    void sendPerNoteAftertouch(int channel, int note, uint32_t value, int group = 0);
    void sendProgramChange(int channel, uint8_t program, uint8_t bankMSB, uint8_t bankLSB, int group = 0);
    
    // MIDI connection state
    bool hasValidMidiPair() const;
    void setMidiConnectionChangedCallback(std::function<void(bool)> callback);
    void setExternalOutputCallback(std::function<void(const libremidi::ump&)> callback);
    void setIncomingNoteCallback(std::function<void(int note, int velocity, bool is_pressed)> callback);
    void setIncomingControlValueCallback(std::function<void(const IncomingControlValue&)> callback);
    
private:
    std::unique_ptr<libremidi::midi_in> midiIn;
    std::unique_ptr<libremidi::midi_out> midiOut;
    std::unique_ptr<libremidi::observer> observer;
    std::unique_ptr<MidiCIManager> midiCIManager;
    
    std::string currentInputDeviceId;
    std::string currentOutputDeviceId;
    
    std::function<void(bool)> midiConnectionChangedCallback;
    std::function<void(uint32_t, const std::string&, const std::string&)> midiCIPropertiesChangedCallback;
    std::function<void()> midiCIDevicesChangedCallback;
    std::function<void(uint32_t, const std::vector<uint8_t>&)> stateSaveCallback;
    bool initialized = false;
    uint32_t local_app_muid = 0;  // Store local application MUID across reinitializations
    
    // Track outgoing SysEx messages to avoid feedback loops
    std::set<std::vector<uint8_t>> recentOutgoingSysEx;
    
    void onMidiInput(libremidi::ump&& packet);
    void dispatch_outgoing_packet(const libremidi::ump& packet);
    bool extract_note_event(const libremidi::ump& packet, int& note, int& velocity, bool& is_pressed) const;
    bool extract_control_value(const libremidi::ump& packet, IncomingControlValue& value) const;
    
    // Helper functions for creating UMP packets
    libremidi::ump createUmpNoteOn(int channel, int note, int velocity);
    libremidi::ump createUmpNoteOff(int channel, int note);
    
    // MIDI-CI helper methods
    void initializeMidiCI();
    void processSysExForMidiCI(uint8_t group, const std::vector<uint8_t>& sysex_data);
    bool sendSysExViaMidi(uint8_t group, const std::vector<uint8_t>& data);
    
    // Connection state helpers
    void updateUIConnectionState();
    void checkAndAutoConnect();
    static std::string normalizeDeviceName(const std::string& deviceName);
    bool previousConnectionState = false;
    
    // SysEx reconstruction state for multi-packet UMP SysEx7
    std::vector<uint8_t> sysex_buffer_;
    bool sysex_in_progress_ = false;
    
    midicci::keyboard::MessageLogger* logger_;
    std::function<void(const libremidi::ump&)> external_output_callback_;
    std::function<void(int, int, bool)> incoming_note_callback_;
    std::function<void(const IncomingControlValue&)> incoming_control_callback_;
};
