#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <optional>

namespace midicci {
namespace commonproperties {

namespace StandardPropertyNames {
    constexpr const char* STATE_LIST = "StateList";
    constexpr const char* ALL_CTRL_LIST = "AllCtrlList";
    constexpr const char* CH_CTRL_LIST = "ChCtrlList";
    constexpr const char* PROGRAM_LIST = "ProgramList";
}

struct MidiCIState {
    std::string title;
    std::string stateId;
    std::optional<std::string> stateRev;
    std::optional<int64_t> timestamp;
    std::optional<std::string> description;
    std::optional<int32_t> size;
    
    MidiCIState(const std::string& title, const std::string& stateId,
                const std::optional<std::string>& stateRev = std::nullopt,
                const std::optional<int64_t>& timestamp = std::nullopt,
                const std::optional<std::string>& description = std::nullopt,
                const std::optional<int32_t>& size = std::nullopt);
};

namespace MidiCIControlType {
    constexpr const char* CC = "cc";
    constexpr const char* CH_PRESS = "chPress";
    constexpr const char* P_PRESS = "pPress";
    constexpr const char* NRPN = "nrpn";
    constexpr const char* RPN = "rpn";
    constexpr const char* P_BEND = "pBend";
    constexpr const char* PNRC = "pnrc";
    constexpr const char* PNAC = "pnac";
    constexpr const char* PNP = "pnp";
}

namespace MidiCIControlTransmit {
    constexpr const char* ABSOLUTE = "absolute";
    constexpr const char* RELATIVE = "relative";
    constexpr const char* BOTH = "both";
    constexpr const char* NONE = "none";
}

namespace MidiCIControlTypeHint {
    constexpr const char* CONTINUOUS = "continuous";
    constexpr const char* MOMENTARY = "momentary";
    constexpr const char* TOGGLE = "toggle";
    constexpr const char* RELATIVE = "relative";
    constexpr const char* VALUE_SELECT = "valueSelect";
}

struct MidiCIControl {
    std::string title;
    std::string ctrlType;
    std::string description;
    std::vector<uint8_t> ctrlIndex;
    std::optional<uint8_t> channel;
    std::optional<uint8_t> priority;
    uint32_t defaultValue;
    std::string transmit;
    std::string recognize;
    int32_t numSigBits;
    std::optional<std::string> paramPath;
    std::optional<std::string> typeHint;
    std::optional<std::string> ctrlMapId;
    std::optional<int32_t> stepCount;
    std::vector<uint32_t> minMax;
    bool defaultCCMap;
    
    MidiCIControl(const std::string& title, const std::string& ctrlType,
                  const std::string& description = "",
                  const std::vector<uint8_t>& ctrlIndex = {0},
                  const std::optional<uint8_t>& channel = std::nullopt,
                  const std::optional<uint8_t>& priority = std::nullopt,
                  uint32_t defaultValue = 0,
                  const std::string& transmit = MidiCIControlTransmit::ABSOLUTE,
                  const std::string& recognize = MidiCIControlTransmit::ABSOLUTE,
                  int32_t numSigBits = 32,
                  const std::optional<std::string>& paramPath = std::nullopt,
                  const std::optional<std::string>& typeHint = std::nullopt,
                  const std::optional<std::string>& ctrlMapId = std::nullopt,
                  const std::optional<int32_t>& stepCount = std::nullopt,
                  const std::vector<uint32_t>& minMax = {0, UINT32_MAX},
                  bool defaultCCMap = false);
};

struct MidiCIProgram {
    std::string title;
    std::vector<uint8_t> bankPC; // minItems = 3, maxItems = 3
    std::optional<std::vector<std::string>> category; // minItems = 1, minLength = 1
    std::optional<std::vector<std::string>> tags; // minItems = 1, minLength = 1
    
    MidiCIProgram(const std::string& title, const std::vector<uint8_t>& bankPC,
                  const std::optional<std::vector<std::string>>& category = std::nullopt,
                  const std::optional<std::vector<std::string>>& tags = std::nullopt);
};

namespace StatePropertyNames {
    constexpr const char* TITLE = "title";
    constexpr const char* STATE_ID = "stateId";
    constexpr const char* STATE_REV = "stateRev";
    constexpr const char* TIMESTAMP = "timestamp";
    constexpr const char* DESCRIPTION = "description";
    constexpr const char* SIZE = "size";
}

namespace ControlPropertyNames {
    constexpr const char* TITLE = "title";
    constexpr const char* DESCRIPTION = "description";
    constexpr const char* CTRL_TYPE = "ctrlType";
    constexpr const char* CTRL_INDEX = "ctrlIndex";
    constexpr const char* CHANNEL = "channel";
    constexpr const char* PRIORITY = "priority";
    constexpr const char* DEFAULT = "default";
    constexpr const char* TRANSMIT = "transmit";
    constexpr const char* RECOGNIZE = "recognize";
    constexpr const char* NUM_SIG_BITS = "numSigBits";
    constexpr const char* PARAM_PATH = "paramPath";
    constexpr const char* TYPE_HINT = "typeHint";
    constexpr const char* CTRL_MAP_ID = "ctrlMapId";
    constexpr const char* STEP_COUNT = "stepCount";
    constexpr const char* MIN_MAX = "minMax";
    constexpr const char* DEFAULT_CC_MAP = "defaultCCMap";
}

namespace ProgramPropertyNames {
    constexpr const char* TITLE = "title";
    constexpr const char* BANK_PC = "bankPC";
    constexpr const char* CATEGORY = "category";
    constexpr const char* TAGS = "tags";
}

class StandardProperties {
public:
    static std::vector<MidiCIState> parseStateList(const std::vector<uint8_t>& data);
    static std::vector<MidiCIControl> parseControlList(const std::vector<uint8_t>& data);
    static std::vector<MidiCIProgram> parseProgramList(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> toJson(const std::vector<MidiCIState>& stateList);
    static std::vector<uint8_t> toJson(const std::vector<MidiCIControl>& controlList);
    static std::vector<uint8_t> toJson(const std::vector<MidiCIProgram>& programList);
};

} // namespace commonproperties

// Forward declaration for MidiCIDevice extension methods
class MidiCIDevice;

// Extension methods for MidiCIDevice (similar to Kotlin extension properties)
namespace StandardPropertiesExtensions {
    // Getter extensions (delegate to ObservablePropertyList getters)
    std::optional<std::vector<commonproperties::MidiCIState>> getStateList(const MidiCIDevice& device);
    std::optional<std::vector<commonproperties::MidiCIControl>> getAllCtrlList(const MidiCIDevice& device);
    std::optional<std::vector<commonproperties::MidiCIControl>> getChCtrlList(const MidiCIDevice& device);
    std::optional<std::vector<commonproperties::MidiCIProgram>> getProgramList(const MidiCIDevice& device);
    
    // Setter extensions (use PropertyHostFacade setPropertyValue)
    void setStateList(MidiCIDevice& device, const std::vector<commonproperties::MidiCIState>& stateList);
    void setAllCtrlList(MidiCIDevice& device, const std::vector<commonproperties::MidiCIControl>& controlList);
    void setChCtrlList(MidiCIDevice& device, const std::vector<commonproperties::MidiCIControl>& controlList);
    void setProgramList(MidiCIDevice& device, const std::vector<commonproperties::MidiCIProgram>& programList);
}

} // namespace midicci