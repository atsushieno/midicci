find_package(Threads REQUIRED)

set(GTEST_SOURCES
    test_ci_factory.cpp
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
    test_ci_device_model_properties.cpp
    #test_final_correlation.cpp
    test_message_serialization.cpp
    test_property_callback.cpp
    test_property_fix.cpp
    test_request_id_correlation.cpp
    test_request_id_correlation_advanced.cpp
    test_request_id_debug.cpp
    test_request_id_valid_muid.cpp
    test_resource_list.cpp
    test_standard_properties_ctrlmaplist.cpp
    test_allctrllist_parser.cpp
    test_allctrllist_messaging.cpp
    test_ump_factory.cpp
    test_ump_retriever.cpp
    test_ump.cpp
    test_ump_translator.cpp
)

add_executable(midicci-gtest ${GTEST_SOURCES})

target_link_libraries(midicci-gtest
    PRIVATE 
        midicci
        midicci-tooling
        gtest_main
        gtest
        Threads::Threads
)

target_include_directories(midicci-gtest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/tooling/include
)

target_compile_features(midicci-gtest PRIVATE cxx_std_20)
