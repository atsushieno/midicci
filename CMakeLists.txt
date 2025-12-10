cmake_minimum_required(VERSION 3.18)

project(midicci
    VERSION 1.0.0
    DESCRIPTION "Cross-platform MIDI 2.0 MIDI-CI session library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (WIN32) #MSVC
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
else ()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
endif ()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

include(GNUInstallDirs)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  libremidi
  GIT_REPOSITORY https://github.com/celtera/libremidi.git
  GIT_TAG v5.3.1
)
FetchContent_MakeAvailable(libremidi)

FetchContent_Declare(
  zlib-ng
  GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
  GIT_TAG 2.3.2
)
set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(zlib-ng)
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples directory not yet implemented
# if(BUILD_EXAMPLES)
#     add_subdirectory(examples)
# endif()

# Qt5/Qt6 detection for optional tools
# Handle Homebrew Qt5 installation paths on macOS
if(APPLE)
    # Check for Homebrew Qt5 installation paths
    if(EXISTS "/opt/homebrew/opt/qt@5")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@5")
    elseif(EXISTS "/usr/local/opt/qt@5")
        list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt@5")
    endif()
endif()

# Try Qt6 first, fallback to Qt5 if not found
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 QUIET COMPONENTS Core Widgets)
endif()

add_subdirectory(tools)

set(CPACK_PACKAGE_NAME "midicci")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
include(CPack)
