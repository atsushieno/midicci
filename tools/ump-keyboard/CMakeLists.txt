# UMP Keyboard Tool
# A Qt-based MIDI keyboard application with MIDI-CI support

find_package(Qt6 COMPONENTS Core Widgets)

# Add external dependencies
include(FetchContent)

# cmidi2 for UMP support
FetchContent_Declare(cmidi2
        GIT_REPOSITORY https://github.com/atsushieno/cmidi2
        GIT_TAG        c232860
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cmidi2)

qt6_standard_project_setup()

# Create a Qt-independent core library with business logic
add_library(midicci-keyboard-core
    src/keyboard_controller.cpp
    src/midi_ci_manager.cpp
    src/message_logger.cpp
)

# Link required libraries to the core library (no Qt dependencies)
target_link_libraries(midicci-keyboard-core
    PUBLIC
    libremidi
    midicci
)

# Include directories for the core library
target_include_directories(midicci-keyboard-core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${cmidi2_SOURCE_DIR}
)

# Create a Qt-specific GUI library
add_library(midicci-keyboard-qt-gui
    src/keyboard_widget.cpp
    src/virtualized_control_list.cpp
    src/main_window.cpp
    src/log_widget.cpp
)

# Link Qt and core to the GUI library
target_link_libraries(midicci-keyboard-qt-gui
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    midicci-keyboard-core
)

# Set up Qt for the GUI library
set_target_properties(midicci-keyboard-qt-gui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Include directories for the GUI library
target_include_directories(midicci-keyboard-qt-gui
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
    midicci-keyboard-qt-gui
    midicci-keyboard-core
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
# Disable it - Claude Code was too stupid to write usable tests.
if (false)
add_test(NAME UMPKeyboardTests COMMAND ump_keyboard_tests)

set_tests_properties(UMPKeyboardTests PROPERTIES
    TIMEOUT 60
)

endif(false)
