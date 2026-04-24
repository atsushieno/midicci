set(MIDICCI_SOURCES
        MidiCIDevice.cpp
        ClientConnection.cpp
        MidiCIConverter.cpp
        CIFactory.cpp
        CIRetrieval.cpp
        Json.cpp
        Message.cpp
        Messenger.cpp
        ObservableProfileList.cpp
        ProfileClientFacade.cpp
        ProfileHostFacade.cpp
        PropertyChunkManager.cpp
        PropertyClientFacade.cpp
        PropertyHostFacade.cpp
        PropertyCommonConverter.cpp
        ObservablePropertyList.cpp
        PropertyPartialUpdater.cpp
        commonproperties/CommonRulesPropertyHelper.cpp
        commonproperties/CommonRulesPropertyService.cpp
        commonproperties/CommonRulesPropertyClient.cpp
        commonproperties/CommonRulesPropertyMetadata.cpp
        commonproperties/FoundationalResources.cpp
        commonproperties/StandardProperties.cpp
        musicdevice/MidiMachineMessageReporter.cpp
        musicdevice/MidiCISession.cpp
        musicdevice/MusicDevice.cpp
)

set(MODULE_NAME midicci)
set(_midicci_install_targets)

if(MIDICCI_BUILD_SHARED)
    add_library(${MODULE_NAME} SHARED ${MIDICCI_SOURCES})
    target_include_directories(${MODULE_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_features(${MODULE_NAME} PUBLIC cxx_std_20)
    target_link_libraries(${MODULE_NAME} PUBLIC umppi_internal PRIVATE zlib)
    set_target_properties(${MODULE_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${MIDICCI_HEADERS}"
    )
    if(WIN32)
        target_compile_definitions(${MODULE_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
        set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
        set_target_properties(${MODULE_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/dll"
            PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
        )
    endif()
    list(APPEND _midicci_install_targets ${MODULE_NAME})
endif()

if(MIDICCI_BUILD_STATIC)
    add_library(${MODULE_NAME}_static STATIC ${MIDICCI_SOURCES})
    target_include_directories(${MODULE_NAME}_static
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_features(${MODULE_NAME}_static PUBLIC cxx_std_20)
    target_link_libraries(${MODULE_NAME}_static PUBLIC umppi_internal PRIVATE zlib)
    set_target_properties(${MODULE_NAME}_static PROPERTIES
        OUTPUT_NAME ${MODULE_NAME}
        POSITION_INDEPENDENT_CODE ON
    )
    if(WIN32)
        set_target_properties(${MODULE_NAME}_static PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/static"
            PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/static"
        )
    endif()
    list(APPEND _midicci_install_targets ${MODULE_NAME}_static)
endif()

# Internal alias for consumers inside this build tree; prefers static when available.
if(MIDICCI_BUILD_STATIC)
    add_library(midicci_internal ALIAS ${MODULE_NAME}_static)
else()
    add_library(midicci_internal ALIAS ${MODULE_NAME})
endif()

if(WIN32)
    if(MIDICCI_BUILD_SHARED)
        install(TARGETS ${MODULE_NAME}
            EXPORT midicci-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/dll
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/midicci
        )
    endif()
    if(MIDICCI_BUILD_STATIC)
        install(TARGETS ${MODULE_NAME}_static
            EXPORT midicci-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
        )
    endif()
else()
    install(TARGETS ${_midicci_install_targets}
        EXPORT midicci-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/midicci
    )
endif()

install(EXPORT midicci-targets
    FILE midicci-targets.cmake
    NAMESPACE midicci::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/midicci
)
