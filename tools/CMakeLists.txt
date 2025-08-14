add_subdirectory(tooling)

# Make Qt5 tool optional - use Qt detection from top-level
if(Qt6_FOUND OR Qt5_FOUND)
    add_subdirectory(qt5-ci-tool)
else()
    message(STATUS "Qt5/Qt6 not found, skipping qt5-ci-tool")
endif()

# Make UMP keyboard tool optional - requires Qt6
if(Qt6_FOUND)
    add_subdirectory(ump-keyboard)
else()
    message(STATUS "Qt6 not found, skipping ump-keyboard")
endif()
