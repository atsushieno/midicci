find_package(Threads REQUIRED)

set(GTEST_SOURCES
    test_ci_factory.cpp
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

add_executable(midicci-gtest ${GTEST_SOURCES})

target_link_libraries(midicci-gtest
    PRIVATE 
        midicci
        gtest_main
        gtest
        Threads::Threads
)

target_include_directories(midicci-gtest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(midicci-gtest PRIVATE cxx_std_20)
