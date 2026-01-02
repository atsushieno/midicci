cmake_minimum_required(VERSION 3.18)

project(midicci
    VERSION 0.1.2
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

function(midicci_fetchcontent_makeavailable_static target_name)
    set(_midicci_prev_build_shared ${BUILD_SHARED_LIBS})
    if(NOT DEFINED _midicci_prev_build_shared)
        set(_midicci_prev_build_shared ON)
    endif()
    set(_midicci_prev_skip_install ${CMAKE_SKIP_INSTALL_RULES})
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CMAKE_SKIP_INSTALL_RULES ON)
    FetchContent_MakeAvailable(${target_name})
    if(DEFINED _midicci_prev_skip_install)
        set(CMAKE_SKIP_INSTALL_RULES ${_midicci_prev_skip_install})
    else()
        unset(CMAKE_SKIP_INSTALL_RULES)
    endif()
    set(BUILD_SHARED_LIBS ${_midicci_prev_build_shared} CACHE BOOL "" FORCE)
endfunction()

# Fetch libremidi for tools / tests, but do not install it
FetchContent_Declare(
    libremidi
    GIT_REPOSITORY https://github.com/celtera/libremidi.git
    GIT_TAG v5.3.1
    EXCLUDE_FROM_ALL
)
midicci_fetchcontent_makeavailable_static(libremidi)

FetchContent_Declare(
    zlib-ng
    GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
    GIT_TAG 2.3.2
    EXCLUDE_FROM_ALL
)
set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Prevent zlib-ng from installing, but otherwise respect its defaults
midicci_fetchcontent_makeavailable_static(zlib-ng)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    # Fetch googletest only for tests, and prevent it from installing
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.12.1
    )
    # Ensure gtest participates in configuration but does not install itself.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
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

# Install public headers only from our tree
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/midicci
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# -----------------
# CPack (Deb/RPM/TXZ)
# -----------------
# Version used for packages (override with -DMIDICCI_VERSION=...)
set(MIDICCI_VERSION "0.1.2" CACHE STRING "Project version for packaging")

set(CPACK_PACKAGE_NAME "midicci")
set(CPACK_PACKAGE_VENDOR "midicci")
set(CPACK_PACKAGE_CONTACT "midicci maintainers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "midicci MIDI-CI tools and libraries")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/atsushieno/midicci")

# Install under /usr in packages regardless of CMAKE_INSTALL_PREFIX
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

# Generators: deb, rpm, and a portable txz archive
set(CPACK_GENERATOR "DEB;RPM;TXZ")

# Debian-specific
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# RPM-specific
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")

# Common metadata
set(CPACK_PACKAGE_VERSION "${MIDICCI_VERSION}")

# Package only our component
set(CPACK_COMPONENTS_ALL midicci)
set(CPACK_COMPONENT_MIDICCI_DISPLAY_NAME "midicci")
set(CPACK_COMPONENT_MIDICCI_DESCRIPTION "MIDICCI MIDI-CI tools, libraries, and headers")

include(CPack)

# -----------------
# Arch Linux helper
# -----------------
set(PACKAGING_ARCH_DIR "${CMAKE_BINARY_DIR}/packaging/arch")
file(MAKE_DIRECTORY "${PACKAGING_ARCH_DIR}")
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PKGBUILD.in
        ${PACKAGING_ARCH_DIR}/PKGBUILD
        @ONLY)
add_custom_target(package-arch-midicci
        COMMAND ${CMAKE_COMMAND} -E echo "PKGBUILD generated at: ${PACKAGING_ARCH_DIR}/PKGBUILD"
        WORKING_DIRECTORY ${PACKAGING_ARCH_DIR}
        COMMENT "Generate Arch Linux PKGBUILD (use makepkg to build)")
