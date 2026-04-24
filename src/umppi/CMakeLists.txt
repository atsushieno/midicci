cmake_minimum_required(VERSION 3.16)

set(UMPPI_SOURCES
    Common.cpp
    Utility.cpp
    DeltaTimeComputer.cpp
    Midi1Message.cpp
    Midi1Music.cpp
    Midi1Reader.cpp
    Midi1Writer.cpp
    Midi1Machine.cpp
    MidiPlayerTimer.cpp
    Ump.cpp
    UmpFactory.cpp
    UmpRetriever.cpp
    UmpTranslator.cpp
    Midi2Track.cpp
)

set(_umppi_install_targets)

if(MIDICCI_BUILD_SHARED)
    add_library(umppi SHARED ${UMPPI_SOURCES})
    target_include_directories(umppi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(umppi PUBLIC cxx_std_20)
    if(WIN32)
        target_compile_definitions(umppi PRIVATE WIN32_LEAN_AND_MEAN)
        set_target_properties(umppi PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
        set_target_properties(umppi PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/dll"
            PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
        )
    endif()
    list(APPEND _umppi_install_targets umppi)
endif()

if(MIDICCI_BUILD_STATIC)
    add_library(umppi_static STATIC ${UMPPI_SOURCES})
    target_include_directories(umppi_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(umppi_static PUBLIC cxx_std_20)
    set_target_properties(umppi_static PROPERTIES
        OUTPUT_NAME umppi
        POSITION_INDEPENDENT_CODE ON
    )
    if(WIN32)
        set_target_properties(umppi_static PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/static"
            PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/$<CONFIG>/lib/static"
        )
    endif()
    list(APPEND _umppi_install_targets umppi_static)
endif()

# Internal alias for consumers inside this build tree; prefers static when available.
if(MIDICCI_BUILD_STATIC)
    add_library(umppi_internal ALIAS umppi_static)
else()
    add_library(umppi_internal ALIAS umppi)
endif()

if(WIN32)
    if(MIDICCI_BUILD_SHARED)
        install(TARGETS umppi
            EXPORT umppi-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/dll
        )
    endif()
    if(MIDICCI_BUILD_STATIC)
        install(TARGETS umppi_static
            EXPORT umppi-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
        )
    endif()
else()
    install(TARGETS ${_umppi_install_targets}
        EXPORT umppi-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../include/umppi
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT umppi-targets
    FILE umppi-targets.cmake
    NAMESPACE umppi::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/umppi
)
