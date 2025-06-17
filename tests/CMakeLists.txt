find_package(Threads REQUIRED)

set(GTEST_SOURCES
    test_property_json_serialization.cpp
    test_message_serialization.cpp
    test_messenger.cpp
    test_profile_manager.cpp
    test_json_serialization.cpp
    test_midi_ci_device_extended.cpp
    test_midi_ci_converter.cpp
    test_property_facades.cpp
    test_profile_facades.cpp
    test_midi_ci_device.cpp
    test_json.cpp
    test_property_common_converter.cpp
    TestCIMediator.cpp
    TestPropertyRules.cpp
)

set(CUSTOM_TEST_SOURCES
    test_main.cpp
)

add_executable(midi-ci-gtest ${GTEST_SOURCES})

target_link_libraries(midi-ci-gtest 
    PRIVATE 
        midi-ci
        gtest_main
        gtest
        Threads::Threads
)

target_include_directories(midi-ci-gtest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(midi-ci-gtest PRIVATE cxx_std_20)

add_executable(midi-ci-tests ${CUSTOM_TEST_SOURCES})

target_link_libraries(midi-ci-tests 
    PRIVATE 
        midi-ci
        Threads::Threads
)

target_include_directories(midi-ci-tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(midi-ci-tests PRIVATE cxx_std_20)

add_test(NAME midi-ci-gtest-tests COMMAND midi-ci-gtest)
add_test(NAME midi-ci-custom-tests COMMAND midi-ci-tests)
