# Tests CMakeLists.txt

# Create a library from the main source files (excluding main.cpp)
add_library(keyboard_core 
    ${CMAKE_CURRENT_LIST_DIR}/../src/keyboard_controller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/midi_ci_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/keyboard_widget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/virtualized_control_list.cpp
)

# Link required libraries to the core library
target_link_libraries(keyboard_core 
    PRIVATE
    Qt6::Core 
    Qt6::Widgets
    libremidi
    midicci
)

# Set up Qt for the core library
set_target_properties(keyboard_core PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Include directories for the core library
target_include_directories(keyboard_core 
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${cmidi2_SOURCE_DIR}
)

# Create the test executables
add_executable(
    midi_feedback_loop_test
    test_midi_feedback_loop.cpp
)

add_executable(
    standard_properties_test
    test_standard_properties.cpp
)

add_executable(
    properties_parsing_test
    test_properties_parsing.cpp
)

add_executable(
    end_to_end_properties_test
    test_end_to_end_properties.cpp
)

# Link the test executables with GoogleTest and our core library
target_link_libraries(
    midi_feedback_loop_test
    PRIVATE
    keyboard_core
    gtest_main
    gtest
    libremidi
    midicci
)

target_link_libraries(
    standard_properties_test
    PRIVATE
    keyboard_core
    gtest_main
    gtest
    libremidi
    midicci
)

target_link_libraries(
    properties_parsing_test
    PRIVATE
    keyboard_core
    gtest_main
    gtest
    libremidi
    midicci
)

target_link_libraries(
    end_to_end_properties_test
    PRIVATE
    keyboard_core
    gtest_main
    gtest
    libremidi
    midicci
)

# Include directories for the tests
target_include_directories(midi_feedback_loop_test 
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${cmidi2_SOURCE_DIR}
)

target_include_directories(standard_properties_test 
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${cmidi2_SOURCE_DIR}
)

target_include_directories(properties_parsing_test 
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${cmidi2_SOURCE_DIR}
)

target_include_directories(end_to_end_properties_test 
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${cmidi2_SOURCE_DIR}
)

# Add the tests to CTest
add_test(NAME MIDIFeedbackLoopTest COMMAND midi_feedback_loop_test)
add_test(NAME StandardPropertiesTest COMMAND standard_properties_test)
add_test(NAME PropertiesParsingTest COMMAND properties_parsing_test)
add_test(NAME EndToEndPropertiesTest COMMAND end_to_end_properties_test)

# Set test properties
set_tests_properties(MIDIFeedbackLoopTest PROPERTIES
    TIMEOUT 60  # 60 seconds timeout
)

set_tests_properties(StandardPropertiesTest PROPERTIES
    TIMEOUT 60  # 60 seconds timeout
)

set_tests_properties(PropertiesParsingTest PROPERTIES
    TIMEOUT 60  # 60 seconds timeout
)

set_tests_properties(EndToEndPropertiesTest PROPERTIES
    TIMEOUT 60  # 60 seconds timeout
)