set(MIDICCI_SOURCES
        midicci/MidiCIDevice.cpp
        midicci/ClientConnection.cpp
        midicci/MidiCIConverter.cpp
        midicci/CIFactory.cpp
        midicci/CIRetrieval.cpp
        midicci/Json.cpp
        midicci/Message.cpp
        midicci/Messenger.cpp
        midicci/ObservableProfileList.cpp
        midicci/ProfileClientFacade.cpp
        midicci/ProfileHostFacade.cpp
        midicci/PropertyChunkManager.cpp
        midicci/PropertyClientFacade.cpp
        midicci/PropertyHostFacade.cpp
        midicci/PropertyCommonConverter.cpp
        midicci/ObservablePropertyList.cpp
        midicci/PropertyPartialUpdater.cpp
        midicci/commonproperties/CommonRulesPropertyHelper.cpp
        midicci/commonproperties/CommonRulesPropertyService.cpp
        midicci/commonproperties/CommonRulesPropertyClient.cpp
        midicci/commonproperties/CommonRulesPropertyMetadata.cpp
        midicci/commonproperties/FoundationalResources.cpp
        midicci/commonproperties/StandardProperties.cpp
        midicci/ump/Ump.cpp
        midicci/ump/UmpFactory.cpp
        midicci/ump/UmpRetriever.cpp
        midicci/ump/UmpTranslator.cpp
        midicci/musicdevice/MidiMachineMessageReporter.cpp
        midicci/musicdevice/MidiCISession.cpp
        midicci/musicdevice/MusicDevice.cpp
)

set(MODULE_NAME midicci)

# Build both shared and static variants
add_library(${MODULE_NAME} SHARED ${MIDICCI_SOURCES})
add_library(${MODULE_NAME}_static STATIC ${MIDICCI_SOURCES})

target_include_directories(${MODULE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${MODULE_NAME}_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(${MODULE_NAME} PUBLIC cxx_std_20)
target_compile_features(${MODULE_NAME}_static PUBLIC cxx_std_20)

target_link_libraries(${MODULE_NAME} PRIVATE zlib)
target_link_libraries(${MODULE_NAME}_static PRIVATE zlib)

if(WIN32)
    target_compile_definitions(${MODULE_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
    set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

set_target_properties(${MODULE_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${MIDICCI_HEADERS}"
)
set_target_properties(${MODULE_NAME}_static PROPERTIES
    OUTPUT_NAME ${MODULE_NAME}
    POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(${MODULE_NAME}_static PUBLIC ZLIB::ZLIB)

# Keep identical base name for both libs; avoid filename collisions on Windows
# by separating their build output directories.
if(WIN32)
    # DLL and its import lib
    set_target_properties(${MODULE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/dll"
        PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
    )
    # Static archive
    set_target_properties(${MODULE_NAME}_static PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/static"
        PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/static"
    )
endif()

# Install both shared and static libraries on all platforms.
# To avoid midicci.lib name collision on Windows, place the DLL import lib
# and the static archive in different subdirectories.
if(WIN32)
    install(TARGETS ${MODULE_NAME}
        EXPORT midicci-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/dll
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/midicci
    )
    install(TARGETS ${MODULE_NAME}_static
        EXPORT midicci-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
    )
else()
    install(TARGETS ${MODULE_NAME} ${MODULE_NAME}_static
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
