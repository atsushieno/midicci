set(MIDI_CI_SOURCES
    midi-ci/core/MidiCIDevice.cpp
    midi-ci/core/ClientConnection.cpp
    midi-ci/core/MidiCIConstants.cpp
    midi-ci/json/Json.cpp
    midi-ci/messages/Message.cpp
    midi-ci/messages/Messenger.cpp
    midi-ci/profiles/ObservableProfileList.cpp
    midi-ci/profiles/ProfileClientFacade.cpp
    midi-ci/profiles/ProfileHostFacade.cpp
    midi-ci/properties/PropertyChunkManager.cpp
    midi-ci/properties/PropertyClientFacade.cpp
    midi-ci/properties/PropertyHostFacade.cpp
    midi-ci/properties/PropertyManager.cpp
    midi-ci/properties/CommonRulesPropertyHelper.cpp
    midi-ci/properties/CommonRulesPropertyClient.cpp
    midi-ci/ump/Ump.cpp
    midi-ci/ump/UmpRetriever.cpp
)

set(MIDI_CI_HEADERS
    ../include/midi-ci/core/MidiCIDevice.hpp
    ../include/midi-ci/core/ClientConnection.hpp
    ../include/midi-ci/core/MidiCIConstants.hpp
    ../include/midi-ci/core/DeviceConfig.hpp
    ../include/midi-ci/messages/Message.hpp
    ../include/midi-ci/messages/Messenger.hpp
    ../include/midi-ci/properties/PropertyManager.hpp
    ../include/midi-ci/properties/PropertyChunkManager.hpp
    ../include/midi-ci/properties/PropertyCommonRules.hpp
    ../include/midi-ci/properties/CommonRulesPropertyHelper.hpp
    ../include/midi-ci/properties/CommonRulesPropertyClient.hpp
    ../include/midi-ci/transport/SysExTransport.hpp
    ../include/midi-ci/json/Json.hpp
    ../include/midi-ci/ump/Ump.hpp
    ../include/midi-ci/ump/UmpRetriever.hpp
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
