#pragma once

// MIDICCI Library - Main Header
// This header provides access to all MIDICCI functionality.
// Note: Headers in the details/ directory are considered unstable and may change.
// Use only this main header for stable API access.

// They are sorted from primitive to complicated
#include "details/Json.hpp"
#include "details/MidiCIConstants.hpp"
#include "details/MidiCIProfile.hpp"
#include "details/MidiCIChannelList.hpp"
#include "details/CIFactory.hpp"
#include "details/CIRetrieval.hpp"
#include "details/MidiCIConverter.hpp"

#include "details/Message.hpp"
#include "details/Messenger.hpp"

#include "details/PropertyValue.hpp"
#include "details/commonproperties/PropertyMetadata.hpp"

#include "details/ObservableProfileList.hpp"

#include "details/MidiCIDeviceConfiguration.hpp"

#include "details/MidiCIDevice.hpp"
#include "details/ClientConnection.hpp"

#include "details/ProfileClientFacade.hpp"
#include "details/ProfileHostFacade.hpp"

// Property management
#include "details/PropertyChunkManager.hpp"
#include "details/PropertyCommonRules.hpp"
#include "details/PropertyCommonConverter.hpp"

// Common profiles
#include "details/commonprofiles/ProfileCommonRules.hpp"

// Common properties
#include "details/PropertyClientFacade.hpp"
#include "details/commonproperties/CommonRulesPropertyHelper.hpp"
#include "details/commonproperties/CommonRulesPropertyClient.hpp"
#include "details/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "details/commonproperties/MidiCIServicePropertyRules.hpp"
#include "details/commonproperties/CommonRulesPropertyService.hpp"

#include "details/ObservablePropertyList.hpp"

#include "details/commonproperties/FoundationalResources.hpp"
#include "details/commonproperties/StandardProperties.hpp"
#include "details/PropertyHostFacade.hpp"

// Music device
#include "details/musicdevice/MidiCISession.hpp"
#include "details/musicdevice/MidiMachineMessageReporter.hpp"
#include "details/musicdevice/MusicDevice.hpp"

// UMP (Universal MIDI Packet) - provided by umppi module
#include "../umppi/umppi.hpp"
