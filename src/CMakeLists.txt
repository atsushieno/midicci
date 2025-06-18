set(MIDI_CI_SOURCES
        midicci/core/MidiCIDevice.cpp
        midicci/core/ClientConnection.cpp
        midicci/core/MidiCIConstants.cpp
        midicci/core/MidiCIConverter.cpp
        midicci/core/CIFactory.cpp
        midicci/core/CIRetrieval.cpp
        midicci/json_ish/Json.cpp
        midicci/messages/Message.cpp
        midicci/messages/Messenger.cpp
        midicci/profiles/ObservableProfileList.cpp
        midicci/profiles/ProfileClientFacade.cpp
        midicci/profiles/ProfileHostFacade.cpp
        midicci/properties/PropertyChunkManager.cpp
        midicci/properties/PropertyClientFacade.cpp
        midicci/properties/PropertyHostFacade.cpp
        midicci/properties/ObservablePropertyList.cpp
        midicci/properties/CommonRulesPropertyHelper.cpp
        midicci/properties/CommonRulesPropertyClient.cpp
        midicci/properties/CommonRulesPropertyMetadata.cpp
        midicci/ump/Ump.cpp
        midicci/ump/UmpRetriever.cpp
)

set(MIDI_CI_HEADERS
        ../include/midicci/core/MidiCIDevice.hpp
        ../include/midicci/core/ClientConnection.hpp
        ../include/midicci/core/MidiCIConstants.hpp
        ../include/midicci/core/CIFactory.hpp
        ../include/midicci/core/MidiCIDeviceConfiguration.hpp
        ../include/midicci/core/MidiCIChannelList.hpp
        ../include/midicci/messages/Message.hpp
        ../include/midicci/messages/Messenger.hpp
        ../include/midicci/properties/ObservablePropertyList.hpp
        ../include/midicci/properties/PropertyChunkManager.hpp
        ../include/midicci/properties/PropertyCommonRules.hpp
        ../include/midicci/properties/CommonRulesPropertyHelper.hpp
        ../include/midicci/properties/CommonRulesPropertyClient.hpp
        ../include/midicci/transport/SysExTransport.hpp
        ../include/midicci/json_ish/Json.hpp
        ../include/midicci/ump/Ump.hpp
        ../include/midicci/ump/UmpRetriever.hpp
)

add_library(midi-ci ${MIDI_CI_SOURCES} ${MIDI_CI_HEADERS})

target_include_directories(midi-ci
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(midi-ci PUBLIC cxx_std_20)

if(WIN32)
    target_compile_definitions(midi-ci PRIVATE WIN32_LEAN_AND_MEAN)
endif()

set_target_properties(midi-ci PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${MIDI_CI_HEADERS}"
)

install(TARGETS midi-ci
    EXPORT midi-ci-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/midi-ci
)

install(EXPORT midi-ci-targets
    FILE midi-ci-targets.cmake
    NAMESPACE midi-ci::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/midi-ci
)
