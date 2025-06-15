find_package(Threads REQUIRED)

set(TEST_SOURCES
    test_main.cpp
    test_midi_ci_device.cpp
)

add_executable(midi-ci-tests ${TEST_SOURCES})

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

add_test(NAME midi-ci-unit-tests COMMAND midi-ci-tests)
