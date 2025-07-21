#pragma once

// MIDICCI Library - Main Header
// This header provides access to all MIDICCI functionality.
// Note: Headers in the details/ directory are considered unstable and may change.
// Use only this main header for stable API access.

// Core functionality
#include <midicci/details/CIFactory.hpp>
#include <midicci/details/CIRetrieval.hpp>
#include <midicci/details/ClientConnection.hpp>
#include <midicci/details/Json.hpp>
#include <midicci/details/Message.hpp>
#include <midicci/details/Messenger.hpp>
#include <midicci/details/MidiCIChannelList.hpp>
#include <midicci/details/MidiCIConstants.hpp>
#include <midicci/details/MidiCIConverter.hpp>
#include <midicci/details/MidiCIDevice.hpp>
#include <midicci/details/MidiCIDeviceConfiguration.hpp>
#include <midicci/details/MidiCIProfile.hpp>

// Observable lists
#include <midicci/details/ObservableProfileList.hpp>
#include <midicci/details/ObservablePropertyList.hpp>

// Profile facades
#include <midicci/details/ProfileClientFacade.hpp>
#include <midicci/details/ProfileHostFacade.hpp>

// Property management
#include <midicci/details/PropertyChunkManager.hpp>
#include <midicci/details/PropertyClientFacade.hpp>
#include <midicci/details/PropertyCommonRules.hpp>
#include <midicci/details/PropertyHostFacade.hpp>

// Common profiles
#include <midicci/details/commonprofiles/ProfileCommonRules.hpp>

// Common properties
#include <midicci/details/commonproperties/CommonRulesPropertyClient.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyHelper.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyMetadata.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyService.hpp>
#include <midicci/details/commonproperties/FoundationalResources.hpp>
#include <midicci/details/commonproperties/MidiCIServicePropertyRules.hpp>
#include <midicci/details/commonproperties/StandardProperties.hpp>

// Music device
#include <midicci/details/musicdevice/MidiCISession.hpp>
#include <midicci/details/musicdevice/MidiMachineMessageReporter.hpp>
#include <midicci/details/musicdevice/MusicDevice.hpp>

// UMP (Universal MIDI Packet)
#include <midicci/details/ump/Ump.hpp>
#include <midicci/details/ump/UmpRetriever.hpp>