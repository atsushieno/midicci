#pragma once

// MIDICCI Library - Main Header
// This header provides access to all MIDICCI functionality.
// Note: Headers in the details/ directory are considered unstable and may change.
// Use only this main header for stable API access.

// They are sorted from primitive to complicated
#include <midicci/details/Json.hpp>
#include <midicci/details/MidiCIConstants.hpp>
#include <midicci/details/MidiCIProfile.hpp>
#include <midicci/details/MidiCIChannelList.hpp>
#include <midicci/details/CIFactory.hpp>
#include <midicci/details/CIRetrieval.hpp>
#include <midicci/details/MidiCIConverter.hpp>

#include <midicci/details/Message.hpp>
#include <midicci/details/Messenger.hpp>

#include <midicci/details/PropertyValue.hpp>
#include <midicci/details/commonproperties/PropertyMetadata.hpp>

#include <midicci/details/ObservableProfileList.hpp>

#include <midicci/details/MidiCIDeviceConfiguration.hpp>

#include <midicci/details/MidiCIDevice.hpp>
#include <midicci/details/ClientConnection.hpp>

#include <midicci/details/ProfileClientFacade.hpp>
#include <midicci/details/ProfileHostFacade.hpp>

// Property management
#include <midicci/details/PropertyChunkManager.hpp>
#include <midicci/details/PropertyCommonRules.hpp>
#include <midicci/details/PropertyCommonConverter.hpp>

// Common profiles
#include <midicci/details/commonprofiles/ProfileCommonRules.hpp>

// Common properties
#include <midicci/details/PropertyClientFacade.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyHelper.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyClient.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyMetadata.hpp>
#include <midicci/details/commonproperties/MidiCIServicePropertyRules.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyService.hpp>

#include <midicci/details/ObservablePropertyList.hpp>

#include <midicci/details/commonproperties/FoundationalResources.hpp>
#include <midicci/details/commonproperties/StandardProperties.hpp>
#include <midicci/details/PropertyHostFacade.hpp>

// Music device
#include <midicci/details/musicdevice/MidiCISession.hpp>
#include <midicci/details/musicdevice/MidiMachineMessageReporter.hpp>
#include <midicci/details/musicdevice/MusicDevice.hpp>
#include <midicci/details/musicdevice/TransportAdapters.hpp>

// UMP (Universal MIDI Packet) - provided by umppi module
#include <umppi/details/Ump.hpp>
#include <umppi/details/UmpFactory.hpp>
#include <umppi/details/UmpRetriever.hpp>
#include <umppi/details/UmpTranslator.hpp>
