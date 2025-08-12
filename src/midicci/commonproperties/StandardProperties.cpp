#include "midicci/midicci.hpp"

namespace midicci {
namespace commonproperties {

// Static metadata initialization
CommonRulesPropertyMetadata StandardProperties::stateListMetadata = []() {
    CommonRulesPropertyMetadata metadata(StandardPropertyNames::STATE_LIST);
    metadata.canSet = PropertySetAccess::NONE;
    metadata.canSubscribe = false;
    metadata.canPaginate = false;
    metadata.columns = {
        {StatePropertyNames::TITLE, "", "State Title"},
        {StatePropertyNames::STATE_ID, "", "State ID"},
        {StatePropertyNames::STATE_REV, "", "State Revision"},
        {StatePropertyNames::TIMESTAMP, "", "UNIX Timestamp"},
        {StatePropertyNames::DESCRIPTION, "", "Description"},
        {StatePropertyNames::SIZE, "", "Byte Size"}
    };
    return metadata;
}();

CommonRulesPropertyMetadata StandardProperties::stateMetadata = []() {
    CommonRulesPropertyMetadata metadata(StandardPropertyNames::STATE);
    metadata.canSubscribe = false;
    metadata.requireResId = true;
    return metadata;
}();

CommonRulesPropertyMetadata StandardProperties::allCtrlListMetadata = []() {
    CommonRulesPropertyMetadata metadata(StandardPropertyNames::ALL_CTRL_LIST);
    metadata.columns = getCtrlListColumns();
    return metadata;
}();

CommonRulesPropertyMetadata StandardProperties::chCtrlListMetadata = []() {
    CommonRulesPropertyMetadata metadata(StandardPropertyNames::CH_CTRL_LIST);
    metadata.columns = getCtrlListColumns();
    return metadata;
}();

CommonRulesPropertyMetadata StandardProperties::programListMetadata = []() {
    CommonRulesPropertyMetadata metadata(StandardPropertyNames::PROGRAM_LIST);
    metadata.columns = {
        {ProgramPropertyNames::TITLE, "", "Program Title"},
        {ProgramPropertyNames::BANK_PC, "", "Bank MSB, LSB and Program Change"},
        {ProgramPropertyNames::CATEGORY, "", "Categories"},
        {ProgramPropertyNames::TAGS, "", "Meta-tags"}
    };
    return metadata;
}();

std::vector<PropertyResourceColumn> StandardProperties::getCtrlListColumns() {
    return {
        {ControlPropertyNames::TITLE, "", "Active Controller Title"},
        {ControlPropertyNames::DESCRIPTION, "", "Description"},
        {ControlPropertyNames::CTRL_TYPE, "", "Type"},
        {ControlPropertyNames::CTRL_INDEX, "", "Controller Message index"},
        {ControlPropertyNames::CHANNEL, "", "MIDI Channel"},
        {ControlPropertyNames::PRIORITY, "", "Priority"},
        {ControlPropertyNames::DEFAULT, "", "Default Value"},
        {ControlPropertyNames::TRANSMIT, "", "Transmit"},
        {ControlPropertyNames::RECOGNIZE, "", "Recognize"},
        {ControlPropertyNames::NUM_SIG_BITS, "", "Number of significant bits"},
        {ControlPropertyNames::TYPE_HINT, "", "Type Hint"},
        {ControlPropertyNames::CTRL_MAP_ID, "", "Control Map Id"},
        {ControlPropertyNames::STEP_COUNT, "", "Step Count"},
        {ControlPropertyNames::MIN_MAX, "", "Min/Max"}
    };
}

MidiCIStateEntry::MidiCIStateEntry(const std::string& title, const std::string& stateId,
                                   const std::optional<std::string>& stateRev,
                                   const std::optional<int64_t>& timestamp,
                                   const std::optional<std::string>& description,
                                   const std::optional<int32_t>& size)
    : title(title), stateId(stateId), stateRev(stateRev), timestamp(timestamp), 
      description(description), size(size) {}

MidiCIControl::MidiCIControl(const std::string& title, const std::string& ctrlType,
                             const std::string& description,
                             const std::vector<uint8_t>& ctrlIndex,
                             const std::optional<uint8_t>& channel,
                             const std::optional<uint8_t>& priority,
                             uint32_t defaultValue,
                             const std::string& transmit,
                             const std::string& recognize,
                             int32_t numSigBits,
                             const std::optional<std::string>& paramPath,
                             const std::optional<std::string>& typeHint,
                             const std::optional<std::string>& ctrlMapId,
                             const std::optional<int32_t>& stepCount,
                             const std::vector<uint32_t>& minMax,
                             bool defaultCCMap)
    : title(title), ctrlType(ctrlType), description(description), ctrlIndex(ctrlIndex),
      channel(channel), priority(priority), defaultValue(defaultValue), transmit(transmit),
      recognize(recognize), numSigBits(numSigBits), paramPath(paramPath), typeHint(typeHint),
      ctrlMapId(ctrlMapId), stepCount(stepCount), minMax(minMax), defaultCCMap(defaultCCMap) {}

MidiCIProgram::MidiCIProgram(const std::string& title, const std::vector<uint8_t>& bankPC,
                             const std::optional<std::vector<std::string>>& category,
                             const std::optional<std::vector<std::string>>& tags)
    : title(title), bankPC(bankPC), category(category), tags(tags) {}

std::vector<MidiCIStateEntry> StandardProperties::parseStateList(const std::vector<uint8_t>& data) {
    std::vector<MidiCIStateEntry> result;
    
    try {
        std::string json_str(data.begin(), data.end());
        auto json = JsonValue::parse(json_str);
        
        if (!json.is_array()) {
            return result;
        }
        
        const auto& json_array = json.as_array();
        for (const auto& item : json_array) {
            if (!item.is_object()) {
                continue;
            }
            
            const auto& obj = item.as_object();
            
            std::string title;
            std::string stateId;
            std::optional<std::string> stateRev;
            std::optional<int64_t> timestamp;
            std::optional<std::string> description;
            std::optional<int32_t> size;
            
            auto it = obj.find(StatePropertyNames::TITLE);
            if (it != obj.end() && it->second.is_string()) {
                title = it->second.as_string();
            }
            
            it = obj.find(StatePropertyNames::STATE_ID);
            if (it != obj.end() && it->second.is_string()) {
                stateId = it->second.as_string();
            }
            
            it = obj.find(StatePropertyNames::STATE_REV);
            if (it != obj.end() && it->second.is_string()) {
                stateRev = it->second.as_string();
            }
            
            it = obj.find(StatePropertyNames::TIMESTAMP);
            if (it != obj.end() && it->second.is_number()) {
                timestamp = static_cast<int64_t>(it->second.as_number());
            }
            
            it = obj.find(StatePropertyNames::DESCRIPTION);
            if (it != obj.end() && it->second.is_string()) {
                description = it->second.as_string();
            }
            
            it = obj.find(StatePropertyNames::SIZE);
            if (it != obj.end() && it->second.is_number()) {
                size = it->second.as_int();
            }
            
            result.emplace_back(title, stateId, stateRev, timestamp, description, size);
        }
    } catch (...) {
        // Return empty result on parse error
    }
    
    return result;
}

std::vector<MidiCIControl> StandardProperties::parseControlList(const std::vector<uint8_t>& data) {
    std::vector<MidiCIControl> result;
    
    try {
        std::string json_str(data.begin(), data.end());
        auto json = JsonValue::parse(json_str);
        
        if (!json.is_array()) {
            return result;
        }
        
        const auto& json_array = json.as_array();
        for (const auto& item : json_array) {
            if (!item.is_object()) {
                continue;
            }
            
            const auto& obj = item.as_object();
            
            std::string title;
            std::string ctrlType;
            std::string description;
            std::vector<uint8_t> ctrlIndex = {0};
            std::optional<uint8_t> channel;
            std::optional<uint8_t> priority;
            uint32_t defaultValue = 0;
            std::string transmit = MidiCIControlTransmit::ABSOLUTE;
            std::string recognize = MidiCIControlTransmit::ABSOLUTE;
            int32_t numSigBits = 32;
            std::optional<std::string> paramPath;
            std::optional<std::string> typeHint;
            std::optional<std::string> ctrlMapId;
            std::optional<int32_t> stepCount;
            std::vector<uint32_t> minMax = {0, UINT32_MAX};
            bool defaultCCMap = false;
            
            auto it = obj.find(ControlPropertyNames::TITLE);
            if (it != obj.end() && it->second.is_string()) {
                title = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::CTRL_TYPE);
            if (it != obj.end() && it->second.is_string()) {
                ctrlType = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::DESCRIPTION);
            if (it != obj.end() && it->second.is_string()) {
                description = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::CTRL_INDEX);
            if (it != obj.end() && it->second.is_array()) {
                const auto& index_array = it->second.as_array();
                ctrlIndex.clear();
                for (const auto& index_val : index_array) {
                    if (index_val.is_number()) {
                        ctrlIndex.push_back(static_cast<uint8_t>(index_val.as_int()));
                    }
                }
                if (ctrlIndex.empty()) {
                    ctrlIndex = {0};
                }
            }
            
            it = obj.find(ControlPropertyNames::CHANNEL);
            if (it != obj.end() && it->second.is_number()) {
                channel = static_cast<uint8_t>(it->second.as_int());
            }
            
            it = obj.find(ControlPropertyNames::PRIORITY);
            if (it != obj.end() && it->second.is_number()) {
                priority = static_cast<uint8_t>(it->second.as_int());
            }
            
            it = obj.find(ControlPropertyNames::DEFAULT);
            if (it != obj.end() && it->second.is_number()) {
                defaultValue = static_cast<uint32_t>(it->second.as_number());
            }
            
            it = obj.find(ControlPropertyNames::TRANSMIT);
            if (it != obj.end() && it->second.is_string()) {
                transmit = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::RECOGNIZE);
            if (it != obj.end() && it->second.is_string()) {
                recognize = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::NUM_SIG_BITS);
            if (it != obj.end() && it->second.is_number()) {
                numSigBits = it->second.as_int();
            }
            
            it = obj.find(ControlPropertyNames::PARAM_PATH);
            if (it != obj.end() && it->second.is_string()) {
                paramPath = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::TYPE_HINT);
            if (it != obj.end() && it->second.is_string()) {
                typeHint = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::CTRL_MAP_ID);
            if (it != obj.end() && it->second.is_string()) {
                ctrlMapId = it->second.as_string();
            }
            
            it = obj.find(ControlPropertyNames::STEP_COUNT);
            if (it != obj.end() && it->second.is_number()) {
                stepCount = it->second.as_int();
            }
            
            it = obj.find(ControlPropertyNames::MIN_MAX);
            if (it != obj.end() && it->second.is_array()) {
                const auto& minmax_array = it->second.as_array();
                minMax.clear();
                for (const auto& val : minmax_array) {
                    if (val.is_number()) {
                        minMax.push_back(static_cast<uint32_t>(val.as_number()));
                    }
                }
                if (minMax.empty()) {
                    minMax = {0, UINT32_MAX};
                }
            }
            
            it = obj.find(ControlPropertyNames::DEFAULT_CC_MAP);
            if (it != obj.end() && it->second.is_bool()) {
                defaultCCMap = it->second.as_bool();
            }
            
            result.emplace_back(title, ctrlType, description, ctrlIndex, channel, priority,
                               defaultValue, transmit, recognize, numSigBits, paramPath,
                               typeHint, ctrlMapId, stepCount, minMax, defaultCCMap);
        }
    } catch (...) {
        // Return empty result on parse error
    }
    
    return result;
}

std::vector<MidiCIProgram> StandardProperties::parseProgramList(const std::vector<uint8_t>& data) {
    std::vector<MidiCIProgram> result;
    
    try {
        std::string json_str(data.begin(), data.end());
        auto json = JsonValue::parse(json_str);
        
        if (!json.is_array()) {
            return result;
        }
        
        const auto& json_array = json.as_array();
        for (const auto& item : json_array) {
            if (!item.is_object()) {
                continue;
            }
            
            const auto& obj = item.as_object();
            
            std::string title;
            std::vector<uint8_t> bankPC;
            std::optional<std::vector<std::string>> category;
            std::optional<std::vector<std::string>> tags;
            
            auto it = obj.find(ProgramPropertyNames::TITLE);
            if (it != obj.end() && it->second.is_string()) {
                title = it->second.as_string();
            }
            
            it = obj.find(ProgramPropertyNames::BANK_PC);
            if (it != obj.end() && it->second.is_array()) {
                const auto& bankpc_array = it->second.as_array();
                for (const auto& val : bankpc_array) {
                    if (val.is_number()) {
                        bankPC.push_back(static_cast<uint8_t>(val.as_int()));
                    }
                }
            }
            
            it = obj.find(ProgramPropertyNames::CATEGORY);
            if (it != obj.end() && it->second.is_array()) {
                const auto& category_array = it->second.as_array();
                std::vector<std::string> cat_vec;
                for (const auto& val : category_array) {
                    if (val.is_string()) {
                        cat_vec.push_back(val.as_string());
                    }
                }
                if (!cat_vec.empty()) {
                    category = cat_vec;
                }
            }
            
            it = obj.find(ProgramPropertyNames::TAGS);
            if (it != obj.end() && it->second.is_array()) {
                const auto& tags_array = it->second.as_array();
                std::vector<std::string> tags_vec;
                for (const auto& val : tags_array) {
                    if (val.is_string()) {
                        tags_vec.push_back(val.as_string());
                    }
                }
                if (!tags_vec.empty()) {
                    tags = tags_vec;
                }
            }
            
            result.emplace_back(title, bankPC, category, tags);
        }
    } catch (...) {
        // Return empty result on parse error
    }
    
    return result;
}

std::vector<uint8_t> StandardProperties::toJson(const std::vector<MidiCIStateEntry>& stateList) {
    JsonArray json_array;
    
    for (const auto& state : stateList) {
        JsonObject obj;
        obj[StatePropertyNames::TITLE] = JsonValue(state.title);
        obj[StatePropertyNames::STATE_ID] = JsonValue(state.stateId);
        
        if (state.stateRev.has_value()) {
            obj[StatePropertyNames::STATE_REV] = JsonValue(state.stateRev.value());
        }
        
        if (state.timestamp.has_value()) {
            obj[StatePropertyNames::TIMESTAMP] = JsonValue(static_cast<double>(state.timestamp.value()));
        }
        
        if (state.description.has_value()) {
            obj[StatePropertyNames::DESCRIPTION] = JsonValue(state.description.value());
        }
        
        if (state.size.has_value()) {
            obj[StatePropertyNames::SIZE] = JsonValue(static_cast<double>(state.size.value()));
        }
        
        json_array.push_back(JsonValue(obj));
    }
    
    JsonValue root(json_array);
    std::string json_str = root.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> StandardProperties::toJson(const std::vector<MidiCIControl>& controlList) {
    JsonArray json_array;
    
    for (const auto& control : controlList) {
        JsonObject obj;
        obj[ControlPropertyNames::TITLE] = JsonValue(control.title);
        obj[ControlPropertyNames::CTRL_TYPE] = JsonValue(control.ctrlType);
        obj[ControlPropertyNames::DESCRIPTION] = JsonValue(control.description);
        
        JsonArray index_array;
        for (uint8_t index : control.ctrlIndex) {
            index_array.push_back(JsonValue(static_cast<double>(index)));
        }
        obj[ControlPropertyNames::CTRL_INDEX] = JsonValue(index_array);
        
        if (control.channel.has_value()) {
            obj[ControlPropertyNames::CHANNEL] = JsonValue(static_cast<double>(control.channel.value()));
        }
        
        if (control.priority.has_value()) {
            obj[ControlPropertyNames::PRIORITY] = JsonValue(static_cast<double>(control.priority.value()));
        }
        
        obj[ControlPropertyNames::DEFAULT] = JsonValue(static_cast<double>(control.defaultValue));
        obj[ControlPropertyNames::TRANSMIT] = JsonValue(control.transmit);
        obj[ControlPropertyNames::RECOGNIZE] = JsonValue(control.recognize);
        obj[ControlPropertyNames::NUM_SIG_BITS] = JsonValue(static_cast<double>(control.numSigBits));
        
        if (control.paramPath.has_value()) {
            obj[ControlPropertyNames::PARAM_PATH] = JsonValue(control.paramPath.value());
        }
        
        if (control.typeHint.has_value()) {
            obj[ControlPropertyNames::TYPE_HINT] = JsonValue(control.typeHint.value());
        }
        
        if (control.ctrlMapId.has_value()) {
            obj[ControlPropertyNames::CTRL_MAP_ID] = JsonValue(control.ctrlMapId.value());
        }
        
        if (control.stepCount.has_value()) {
            obj[ControlPropertyNames::STEP_COUNT] = JsonValue(static_cast<double>(control.stepCount.value()));
        }
        
        JsonArray minmax_array;
        for (uint32_t val : control.minMax) {
            minmax_array.push_back(JsonValue(static_cast<double>(val)));
        }
        obj[ControlPropertyNames::MIN_MAX] = JsonValue(minmax_array);
        
        obj[ControlPropertyNames::DEFAULT_CC_MAP] = JsonValue(control.defaultCCMap);
        
        json_array.push_back(JsonValue(obj));
    }
    
    JsonValue root(json_array);
    std::string json_str = root.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> StandardProperties::toJson(const std::vector<MidiCIProgram>& programList) {
    JsonArray json_array;
    
    for (const auto& program : programList) {
        JsonObject obj;
        obj[ProgramPropertyNames::TITLE] = JsonValue(program.title);
        
        JsonArray bankpc_array;
        for (uint8_t val : program.bankPC) {
            bankpc_array.push_back(JsonValue(static_cast<double>(val)));
        }
        obj[ProgramPropertyNames::BANK_PC] = JsonValue(bankpc_array);
        
        if (program.category.has_value()) {
            JsonArray category_array;
            for (const auto& cat : program.category.value()) {
                category_array.push_back(JsonValue(cat));
            }
            obj[ProgramPropertyNames::CATEGORY] = JsonValue(category_array);
        }
        
        if (program.tags.has_value()) {
            JsonArray tags_array;
            for (const auto& tag : program.tags.value()) {
                tags_array.push_back(JsonValue(tag));
            }
            obj[ProgramPropertyNames::TAGS] = JsonValue(tags_array);
        }
        
        json_array.push_back(JsonValue(obj));
    }
    
    JsonValue root(json_array);
    std::string json_str = root.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

namespace StandardPropertiesExtensions {

std::optional<std::vector<commonproperties::MidiCIStateEntry>> getStateList(const ObservablePropertyList& properties) {
    auto values = properties.getValues();
    auto it = std::find_if(values.begin(), values.end(),
                           [](const PropertyValue &pv) { return pv.id == commonproperties::StandardPropertyNames::STATE_LIST; });
    if (it != values.end())
        return commonproperties::StandardProperties::parseStateList(it->body);
    return std::nullopt;
}

std::optional<std::vector<commonproperties::MidiCIControl>> getAllCtrlList(const ObservablePropertyList& properties) {
    auto values = properties.getValues();
    auto it = std::find_if(values.begin(), values.end(),
                           [](const PropertyValue& pv) { return pv.id == commonproperties::StandardPropertyNames::ALL_CTRL_LIST; });
    if (it != values.end())
        return commonproperties::StandardProperties::parseControlList(it->body);
    return std::nullopt;
}

std::optional<std::vector<commonproperties::MidiCIControl>> getChCtrlList(const ObservablePropertyList& properties) {
    auto values = properties.getValues();
    auto it = std::find_if(values.begin(), values.end(),
                           [](const PropertyValue& pv) { return pv.id == commonproperties::StandardPropertyNames::CH_CTRL_LIST; });
    if (it != values.end())
        return commonproperties::StandardProperties::parseControlList(it->body);
    return std::nullopt;
}

std::optional<std::vector<commonproperties::MidiCIProgram>> getProgramList(const ObservablePropertyList& properties) {
    auto values = properties.getValues();
    auto it = std::find_if(values.begin(), values.end(),
                           [](const PropertyValue& pv) { return pv.id == commonproperties::StandardPropertyNames::PROGRAM_LIST; });
    if (it != values.end())
        return commonproperties::StandardProperties::parseProgramList(it->body);
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> getState(const ObservablePropertyList& properties, const std::string& stateId) {
    auto values = properties.getValues();
    auto it = std::find_if(values.begin(), values.end(),
                           [&stateId](const PropertyValue& pv) { 
                               return pv.id == commonproperties::StandardPropertyNames::STATE && pv.resId == stateId; 
                           });
    if (it != values.end())
        return it->body;
    return std::nullopt;
}

// MidiCIDevice getters (delegate to ObservablePropertyList getters)
std::optional<std::vector<commonproperties::MidiCIStateEntry>> getStateList(const MidiCIDevice& device) {
    auto &props = device.get_property_host_facade().get_properties();
    return getStateList(props);
}

std::optional<std::vector<commonproperties::MidiCIControl>> getAllCtrlList(const MidiCIDevice& device) {
    auto &props = device.get_property_host_facade().get_properties();
    return getAllCtrlList(props);
}

std::optional<std::vector<commonproperties::MidiCIControl>> getChCtrlList(const MidiCIDevice& device) {
    auto &props = device.get_property_host_facade().get_properties();
    return getChCtrlList(props);
}

std::optional<std::vector<commonproperties::MidiCIProgram>> getProgramList(const MidiCIDevice& device) {
    auto &props = device.get_property_host_facade().get_properties();
    return getProgramList(props);
}

std::optional<std::vector<uint8_t>> getState(const MidiCIDevice& device, const std::string& stateId) {
    auto &props = device.get_property_host_facade().get_properties();
    return getState(props, stateId);
}

// MidiCIDevice setters (use PropertyHostFacade setPropertyValue)
void setStateList(MidiCIDevice& device, const std::optional<std::vector<commonproperties::MidiCIStateEntry>>& stateList) {
    auto json_data = commonproperties::StandardProperties::toJson(stateList.value_or(std::vector<commonproperties::MidiCIStateEntry>{}));
    device.get_property_host_facade().setPropertyValue(
        commonproperties::StandardPropertyNames::STATE_LIST, 
        "", // empty resId
        json_data, 
        false // not partial
    );
}

void setAllCtrlList(MidiCIDevice& device, const std::optional<std::vector<commonproperties::MidiCIControl>>& controlList) {
    auto json_data = commonproperties::StandardProperties::toJson(controlList.value_or(std::vector<commonproperties::MidiCIControl>{}));
    device.get_property_host_facade().setPropertyValue(
        commonproperties::StandardPropertyNames::ALL_CTRL_LIST, 
        "", // empty resId
        json_data, 
        false // not partial
    );
}

void setChCtrlList(MidiCIDevice& device, const std::optional<std::vector<commonproperties::MidiCIControl>>& controlList) {
    auto json_data = commonproperties::StandardProperties::toJson(controlList.value_or(std::vector<commonproperties::MidiCIControl>{}));
    device.get_property_host_facade().setPropertyValue(
        commonproperties::StandardPropertyNames::CH_CTRL_LIST, 
        "", // empty resId
        json_data, 
        false // not partial
    );
}

void setProgramList(MidiCIDevice& device, const std::optional<std::vector<commonproperties::MidiCIProgram>>& programList) {
    auto json_data = commonproperties::StandardProperties::toJson(programList.value_or(std::vector<commonproperties::MidiCIProgram>{}));
    device.get_property_host_facade().setPropertyValue(
        commonproperties::StandardPropertyNames::PROGRAM_LIST, 
        "", // empty resId
        json_data, 
        false // not partial
    );
}

void setState(MidiCIDevice& device, const std::string& stateId, const std::vector<uint8_t>& data) {
    device.get_property_host_facade().setPropertyValue(
        commonproperties::StandardPropertyNames::STATE,
        stateId,
        data,
        false // not partial
    );
}

}

} // namespace commonproperties
} // namespace midicci