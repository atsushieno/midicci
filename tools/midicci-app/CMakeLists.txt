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
    ../ump-keyboard/src/keyboard_controller.cpp
    ../ump-keyboard/src/midi_ci_manager.cpp
    ../ump-keyboard/src/message_logger.cpp
)

# ------------------------------------------------------------------------------
# Additional dependencies for shared keyboard controller sources
# ------------------------------------------------------------------------------
FetchContent_Declare(cmidi2
    GIT_REPOSITORY https://github.com/atsushieno/cmidi2
    GIT_TAG 0.9.3
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cmidi2)

target_include_directories(midicci-app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/../ump-keyboard/src
    ${CMAKE_SOURCE_DIR}/tools/tooling/include
    ${CMAKE_SOURCE_DIR}/external/libremidi/include
    ${cmidi2_SOURCE_DIR}
    ${midicci_SOURCE_DIR}/include
)

target_link_libraries(midicci-app PRIVATE
    midicci-imgui-support
    midicci-tooling
    midicci
    libremidi::libremidi
    ${BACKEND_LIBRARIES}
)

if(APPLE)
    target_link_libraries(midicci-app PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(midicci-app PRIVATE X11::X11)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(midicci-app PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install midicci-app executable only; dependencies are not installed
install(TARGETS midicci-app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
