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
    Midi2Track.cpp
    Midi2Music.cpp
)

add_library(umppi SHARED ${UMPPI_SOURCES})
target_include_directories(umppi PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(umppi PUBLIC cxx_std_20)

add_library(umppi_static STATIC ${UMPPI_SOURCES})
target_include_directories(umppi_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(umppi_static PUBLIC cxx_std_20)

set_target_properties(umppi_static PROPERTIES OUTPUT_NAME umppi)

install(TARGETS umppi umppi_static
    EXPORT umppi-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/umppi
    DESTINATION include
)

install(EXPORT umppi-targets
    FILE umppi-targets.cmake
    NAMESPACE umppi::
    DESTINATION lib/cmake/umppi
)
