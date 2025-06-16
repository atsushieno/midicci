add_subdirectory(ci-tool)

# Make Qt5 tool optional
find_package(Qt5 5.15 QUIET)
if(Qt5_FOUND)
    add_subdirectory(qt5-ci-tool)
else()
    message(STATUS "Qt5 not found, skipping qt5-ci-tool")
endif()
