cmake_minimum_required(VERSION 3.20)

project(midi-ci-tool
    VERSION 1.0.0
    DESCRIPTION "MIDI-CI Tool - Proof of concept application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

set(CI_TOOL_SOURCES
    src/main.cpp
    src/CIDeviceManager.cpp
    src/MidiDeviceManager.cpp
    src/CIDeviceModel.cpp
    src/ClientConnectionModel.cpp
    src/CIToolRepository.cpp
    src/MidiCIProfileState.cpp
)

add_executable(midi-ci-tool ${CI_TOOL_SOURCES})

target_link_libraries(midi-ci-tool 
    PRIVATE 
        midi-ci
)

target_compile_features(midi-ci-tool PRIVATE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(midi-ci-tool PRIVATE 
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

install(TARGETS midi-ci-tool
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
