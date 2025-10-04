add_subdirectory(tooling)

if(Qt6_FOUND OR Qt5_FOUND)
    add_subdirectory(qt5-ci-tool)
else()
    message(STATUS "Qt5/Qt6 not found, skipping qt5-ci-tool")
endif()

if(Qt6_FOUND)
    add_subdirectory(ump-keyboard)
else()
    message(STATUS "Qt6 not found, skipping ump-keyboard")
endif()

add_subdirectory(midicci-keyboard-imgui)
