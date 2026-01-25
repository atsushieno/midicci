cmake_minimum_required(VERSION 3.18)

project(midicci-app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)
include(FetchContent)

# ------------------------------------------------------------------------------
# Select window backend (SDL3 > SDL2 > GLFW)
# ------------------------------------------------------------------------------
set(BACKEND_FOUND FALSE)
set(BACKEND_TYPE "")
set(BACKEND_LIBRARIES "")

find_package(SDL3 QUIET)
if(SDL3_FOUND)
    set(BACKEND_FOUND TRUE)
    set(BACKEND_TYPE "SDL3")
    set(BACKEND_LIBRARIES SDL3::SDL3)
    message(STATUS "midicci-app: using SDL3 backend")
    add_compile_definitions(USE_SDL3_BACKEND)
endif()

if(NOT BACKEND_FOUND)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        set(BACKEND_FOUND TRUE)
        set(BACKEND_TYPE "SDL2")
        set(BACKEND_LIBRARIES SDL2::SDL2)
        message(STATUS "midicci-app: using SDL2 backend")
        add_compile_definitions(USE_SDL2_BACKEND)
    endif()
endif()

if(NOT BACKEND_FOUND)
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        set(BACKEND_FOUND TRUE)
        set(BACKEND_TYPE "GLFW")
        set(BACKEND_LIBRARIES glfw)
        message(STATUS "midicci-app: using GLFW backend (system)")
        add_compile_definitions(USE_GLFW_BACKEND)
    else()
        message(STATUS "midicci-app: fetching GLFW via CPM")
        CPMAddPackage(
            NAME glfw
            GIT_TAG 3.4
            GIT_REPOSITORY https://github.com/glfw/glfw
            OPTIONS
                "BUILD_SHARED_LIBS OFF"
                "GLFW_BUILD_SHARED_LIBS OFF"
                "GLFW_BUILD_EXAMPLES OFF"
                "GLFW_BUILD_TESTS OFF"
                "GLFW_BUILD_DOCS OFF"
        )
        if(glfw_ADDED)
            set(BACKEND_FOUND TRUE)
            set(BACKEND_TYPE "GLFW")
            set(BACKEND_LIBRARIES glfw)
            message(STATUS "midicci-app: using GLFW backend (fetched)")
            add_compile_definitions(USE_GLFW_BACKEND)
        endif()
    endif()
endif()

if(NOT BACKEND_FOUND)
    message(WARNING "midicci-app: no windowing backend available, skipping target")
    return()
endif()

# ------------------------------------------------------------------------------
# ImGui dependency
# ------------------------------------------------------------------------------
if(NOT DEFINED imgui_SOURCE_DIR)
    CPMAddPackage(
        NAME imgui
        GIT_TAG v1.92.3
        GIT_REPOSITORY https://github.com/ocornut/imgui
        DOWNLOAD_ONLY YES
    )
endif()

if(NOT TARGET imgui)
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )

    if(BACKEND_TYPE STREQUAL "SDL3")
        list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
    elseif(BACKEND_TYPE STREQUAL "SDL2")
        list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp)
    elseif(BACKEND_TYPE STREQUAL "GLFW")
        list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
    endif()

    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC ${BACKEND_LIBRARIES})

    if(APPLE)
        target_link_libraries(imgui PUBLIC "-framework OpenGL")
    elseif(UNIX)
        find_package(OpenGL REQUIRED)
        target_link_libraries(imgui PUBLIC OpenGL::GL)
    elseif(WIN32)
        target_link_libraries(imgui PUBLIC opengl32)
    endif()
endif()

# ------------------------------------------------------------------------------
# portable-file-dialogs dependency
# ------------------------------------------------------------------------------
if(NOT DEFINED portable-file-dialogs_SOURCE_DIR)
    CPMAddPackage(
        NAME portable-file-dialogs
        GIT_TAG c12ea8c
        GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs
        DOWNLOAD_ONLY YES
    )
endif()

# ------------------------------------------------------------------------------
# Shared ImGui helpers
# ------------------------------------------------------------------------------
add_library(midicci-imgui-support STATIC
    src/imgui/PlatformBackend.cpp
)
target_include_directories(midicci-imgui-support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(midicci-imgui-support PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(midicci-imgui-support PUBLIC
    imgui
    ${BACKEND_LIBRARIES}
    ${OpenGL_LIBRARIES}
)
target_compile_features(midicci-imgui-support PUBLIC cxx_std_20)

# ------------------------------------------------------------------------------
# midicci-app executable
# ------------------------------------------------------------------------------
add_executable(midicci-app
    src/main.cpp
    src/App.cpp
    src/keyboard/KeyboardPanel.cpp
    src/keyboard/MidiKeyboard.cpp
    src/inspector/InspectorPanel.cpp
    src/local_device/LocalDevicePanel.cpp
    src/keyboard_controller.cpp
    src/midi_ci_manager.cpp
    src/message_logger.cpp
)
set(MIDICCI_APP_TARGETS midicci-app)

if(APPLE)
    add_executable(midicci-app-bundle MACOSX_BUNDLE
        src/main.cpp
        src/App.cpp
        src/keyboard/KeyboardPanel.cpp
        src/keyboard/MidiKeyboard.cpp
        src/inspector/InspectorPanel.cpp
        src/local_device/LocalDevicePanel.cpp
        src/keyboard_controller.cpp
        src/midi_ci_manager.cpp
        src/message_logger.cpp
    )
    list(APPEND MIDICCI_APP_TARGETS midicci-app-bundle)
endif()

# ------------------------------------------------------------------------------
# Additional dependencies for shared keyboard controller sources
# ------------------------------------------------------------------------------

foreach(app_target IN LISTS MIDICCI_APP_TARGETS)
    target_include_directories(${app_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboard
        ${CMAKE_SOURCE_DIR}/tools/tooling/include
        ${CMAKE_SOURCE_DIR}/external/libremidi/include
        ${CMAKE_SOURCE_DIR}/include
        ${midicci_SOURCE_DIR}/include
        ${portable-file-dialogs_SOURCE_DIR}
    )

    target_link_libraries(${app_target} PRIVATE
        midicci-imgui-support
        midicci-tooling
        midicci_static
        libremidi::libremidi
        zlib-ng
        ${BACKEND_LIBRARIES}
    )
endforeach()

if(APPLE)
    foreach(app_target IN LISTS MIDICCI_APP_TARGETS)
        target_link_libraries(${app_target} PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
        )
    endforeach()
elseif(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    find_package(X11 REQUIRED)
    target_link_libraries(midicci-app PRIVATE X11::X11)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(app_target IN LISTS MIDICCI_APP_TARGETS)
        target_compile_options(${app_target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()

# Configure bundle properties on macOS
if(APPLE)
    set(MIDICCI_APP_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/MidicciApp-Info.plist")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${MIDICCI_APP_INFO_PLIST} @ONLY)
    set_target_properties(midicci-app-bundle PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${MIDICCI_APP_INFO_PLIST}
        OUTPUT_NAME "midicci-app"
    )
endif()

install(TARGETS midicci-app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install bundle on macOS to separate component
if(APPLE AND TARGET midicci-app-bundle)
    install(TARGETS midicci-app-bundle
        BUNDLE DESTINATION .
        COMPONENT midicci_app)
endif()

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------
enable_testing()

add_executable(midicci-app-tests
    tests/test_midi_feedback_loop.cpp
    tests/test_standard_properties.cpp
    tests/test_properties_parsing.cpp
    tests/test_end_to_end_properties.cpp
    tests/test_allctrllist_ordering.cpp
    tests/test_blank_titles_issue.cpp
    tests/test_allctrllist_timing.cpp
    tests/test_ordering_bug_analysis.cpp
)

add_library(midicci-keyboard-core STATIC
    src/keyboard_controller.cpp
    src/midi_ci_manager.cpp
    src/message_logger.cpp
)

target_link_libraries(midicci-keyboard-core
    PUBLIC
        libremidi::libremidi
        midicci_static
        umppi_static
)

target_include_directories(midicci-keyboard-core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(midicci-app-tests
    PRIVATE
    midicci-keyboard-core
    gtest_main
    gtest
    libremidi::libremidi
    midicci_static
)

target_include_directories(midicci-app-tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME MidiCIAppTests COMMAND midicci-app-tests)

set_tests_properties(MidiCIAppTests PROPERTIES
    TIMEOUT 60
)
