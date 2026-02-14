cmake_minimum_required(VERSION 3.18)

project(midicci
    VERSION 0.2.0
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
option(MIDICCI_SKIP_TOOLS "Build only midicci libraries" OFF)
option(MIDICCI_ENABLE_WINMIDI "Enable Windows MIDI Services backend integration (Windows only)" ${WIN32})
set(MIDICCI_WINMIDI_HEADERS_ARCHIVE "${CMAKE_CURRENT_LIST_DIR}/external/Microsoft.Windows.Devices.Midi2.1.0.14-rc.1.209.nupkg"
        CACHE FILEPATH "Path to the Windows MIDI Services header package used by libremidi")

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
if(WIN32 AND MIDICCI_ENABLE_WINMIDI)
    if(NOT LIBREMIDI_WINMIDI_HEADERS_ZIP)
        if(NOT MIDICCI_WINMIDI_HEADERS_ARCHIVE)
            message(FATAL_ERROR "MIDICCI_ENABLE_WINMIDI is ON but MIDICCI_WINMIDI_HEADERS_ARCHIVE is not specified.")
        endif()
        set(_midicci_winmidi_archive "${MIDICCI_WINMIDI_HEADERS_ARCHIVE}")
        if(NOT IS_ABSOLUTE "${_midicci_winmidi_archive}")
            cmake_path(SET _midicci_winmidi_archive NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/${_midicci_winmidi_archive}")
        endif()
        if(NOT EXISTS "${_midicci_winmidi_archive}")
            message(FATAL_ERROR
                    "Expected Windows MIDI Services package at ${_midicci_winmidi_archive}.\n"
                    "Place the NuGet archive under external/ or set MIDICCI_WINMIDI_HEADERS_ARCHIVE accordingly.")
        endif()
        set(LIBREMIDI_WINMIDI_HEADERS_ZIP "${_midicci_winmidi_archive}" CACHE FILEPATH "Path to the Windows MIDI Services header package used by libremidi" FORCE)
    endif()
endif()

FetchContent_Declare(
    libremidi
    GIT_REPOSITORY https://github.com/celtera/libremidi.git
    GIT_TAG v5.4.1
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

add_subdirectory(src/umppi)
add_subdirectory(src/midicci)

if(NOT MIDICCI_SKIP_TOOLS)
    add_subdirectory(tools)
endif()

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
    if(NOT MIDICCI_SKIP_TOOLS)
        add_subdirectory(tests)
    endif()
endif()

# Install public headers only from our tree
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/midicci
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# -----------------
# CPack (Deb/RPM/TXZ)
# -----------------
# Version used for packages (override with -DMIDICCI_VERSION=...)
set(MIDICCI_VERSION "0.2.0" CACHE STRING "Project version for packaging")

set(CPACK_PACKAGE_NAME "midicci")
set(CPACK_PACKAGE_VENDOR "midicci")
set(CPACK_PACKAGE_CONTACT "midicci maintainers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "midicci MIDI-CI tools and libraries")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/atsushieno/midicci")

# Install under /usr in packages regardless of CMAKE_INSTALL_PREFIX on Linux
if(UNIX AND NOT APPLE)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()

# Generators per platform
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    find_program(MIDICCI_MAKENSIS_EXECUTABLE NAMES makensis)
    if(MIDICCI_MAKENSIS_EXECUTABLE)
        set(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
        set(CPACK_NSIS_DISPLAY_NAME "MIDICCI MIDI-CI Tools")
        set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
        set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
        set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
        set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
        set(CPACK_NSIS_MODIFY_PATH OFF)
    endif()
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "MIDICCI_${CPACK_PACKAGE_VERSION}")
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE OFF)
    set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK OFF)
else()
    set(CPACK_GENERATOR "DEB;RPM;TXZ")
endif()

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

# Package components per-platform; DMG should only contain the app bundle
if(APPLE)
    set(CPACK_COMPONENTS_ALL midicci_app)
    set(CPACK_COMPONENT_MIDICCI_APP_DISPLAY_NAME "midicci app bundle")
    set(CPACK_COMPONENT_MIDICCI_APP_DESCRIPTION "MIDICCI macOS desktop application bundle")
else()
    set(CPACK_COMPONENTS_ALL midicci)
    set(CPACK_COMPONENT_MIDICCI_DISPLAY_NAME "midicci")
    set(CPACK_COMPONENT_MIDICCI_DESCRIPTION "MIDICCI MIDI-CI tools, libraries, and headers")
endif()

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
