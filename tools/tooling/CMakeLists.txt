cmake_minimum_required(VERSION 3.18)

project(midicci-tooling
    VERSION 1.0.0
    DESCRIPTION "MIDICCI MIDI-CI Tool - Proof of concept application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/midicci/tooling
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

set(MIDICCI_TOOLING_SOURCES
    src/CIDeviceManager.cpp
    src/MidiDeviceManager.cpp
    src/CIDeviceModel.cpp
    src/ClientConnectionModel.cpp
    src/CIToolRepository.cpp
    src/MidiCIProfileState.cpp
)

set(MIDICCI_CUI_SOURCES
    src/main.cpp
)

add_library(midicci-tooling STATIC ${MIDICCI_TOOLING_SOURCES})
target_link_libraries(midicci-tooling 
    PRIVATE
        midicci
        libremidi
)

add_executable(midicci-cui ${MIDICCI_CUI_SOURCES})

target_link_libraries(midicci-cui
    PRIVATE
        midicci
        midicci-tooling
)

add_dependencies(midicci-cui midicci)

target_compile_features(midicci-tooling PRIVATE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(midicci-tooling PRIVATE 
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

install(TARGETS midicci-tooling
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
