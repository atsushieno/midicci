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
        midicci/propertycommonrules/PropertyChunkManager.cpp
        midicci/propertycommonrules/PropertyClientFacade.cpp
        midicci/propertycommonrules/PropertyHostFacade.cpp
        midicci/propertycommonrules/ObservablePropertyList.cpp
        midicci/propertycommonrules/CommonRulesPropertyHelper.cpp
        midicci/propertycommonrules/CommonRulesPropertyService.cpp
        midicci/propertycommonrules/CommonRulesPropertyClient.cpp
        midicci/propertycommonrules/CommonRulesPropertyMetadata.cpp
        midicci/ump/Ump.cpp
        midicci/ump/UmpRetriever.cpp
)

set(MODULE_NAME midicci)

add_library(${MODULE_NAME} ${MIDICCI_SOURCES})

target_include_directories(${MODULE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(${MODULE_NAME} PUBLIC cxx_std_20)

if(WIN32)
    target_compile_definitions(${MODULE_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
    if(BUILD_SHARED_LIBS)
        set_target_properties(${MODULE_NAME} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    endif()
endif()

set_target_properties(${MODULE_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${MIDICCI_HEADERS}"
)

install(TARGETS ${MODULE_NAME}
    EXPORT midicci-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/midicci
)

install(EXPORT midicci-targets
    FILE midicci-targets.cmake
    NAMESPACE midicci::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/midicci
)
