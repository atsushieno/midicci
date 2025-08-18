# UMP Keyboard Tool
# A Qt-based MIDI keyboard application with MIDI-CI support

# Find Qt6 for GUI components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Add external dependencies
include(FetchContent)

# cmidi2 for UMP support
FetchContent_Declare(cmidi2
        GIT_REPOSITORY https://github.com/atsushieno/cmidi2
        GIT_TAG        55fc50a
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cmidi2)

qt6_standard_project_setup()

# Create a library from the main source files (excluding main.cpp)
add_library(midicci-keyboard-core 
    src/keyboard_controller.cpp
    src/midi_ci_manager.cpp
    src/keyboard_widget.cpp
    src/virtualized_control_list.cpp
    src/main_window.cpp
    src/log_widget.cpp
    src/message_logger.cpp
)

# Link required libraries to the core library
target_link_libraries(midicci-keyboard-core 
    PRIVATE
    Qt6::Core 
    Qt6::Widgets
    libremidi
    midicci
)

# Set up Qt for the core library
set_target_properties(midicci-keyboard-core PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Include directories for the core library
target_include_directories(midicci-keyboard-core 
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${cmidi2_SOURCE_DIR}
)

# Create the main executable
add_executable(midicci-keyboard
    src/main.cpp
)

target_link_libraries(midicci-keyboard 
    Qt6::Core
    Qt6::Widgets
    midicci-keyboard-core
    libremidi
    midicci
)

target_include_directories(midicci-keyboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${cmidi2_SOURCE_DIR}
)

# Enable testing
enable_testing()

# Create unified test executable
add_executable(ump_keyboard_tests
    tests/test_midi_feedback_loop.cpp
    tests/test_standard_properties.cpp
    tests/test_properties_parsing.cpp
    tests/test_end_to_end_properties.cpp
    tests/test_allctrllist_ordering.cpp
    tests/test_blank_titles_issue.cpp
    tests/test_allctrllist_timing.cpp
    tests/test_ordering_bug_analysis.cpp
)

target_link_libraries(ump_keyboard_tests
    PRIVATE
    midicci-keyboard-core
    gtest_main
    gtest
    libremidi
    midicci
)

target_include_directories(ump_keyboard_tests 
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${cmidi2_SOURCE_DIR}
)

# Add the unified test to CTest
add_test(NAME UMPKeyboardTests COMMAND ump_keyboard_tests)

set_tests_properties(UMPKeyboardTests PROPERTIES
    TIMEOUT 60
)