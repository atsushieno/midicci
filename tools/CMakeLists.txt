add_subdirectory(tooling)

# Make Qt5 tool optional - use Qt detection from top-level
if(Qt6_FOUND OR Qt5_FOUND)
    add_subdirectory(qt5-ci-tool)
else()
    message(STATUS "Qt5/Qt6 not found, skipping qt5-ci-tool")
endif()
