# MIDICCI Keyboard ImGui Tool
# An ImGui-based MIDI keyboard application with MIDI-CI support

cmake_minimum_required(VERSION 3.18)

# Download CPM.cmake if not already present
set(CPM_DOWNLOAD_VERSION 0.38.1)
if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CMAKE_BINARY_DIR}/cmake/CPM.cmake
    )
endif()
include(${CMAKE_BINARY_DIR}/cmake/CPM.cmake)

# Try to find backends in order: SDL3 > SDL2 > GLFW
set(BACKEND_FOUND FALSE)

# Try SDL3
find_package(SDL3 QUIET)
if(SDL3_FOUND)
    set(BACKEND_FOUND TRUE)
    set(BACKEND_TYPE "SDL3")
    set(BACKEND_LIBRARIES SDL3::SDL3)
    add_compile_definitions(IMGUI_BACKEND_SDL3)
    message(STATUS "midicci-keyboard-imgui: Using SDL3 backend")
endif()

# Try SDL2
if(NOT BACKEND_FOUND)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        set(BACKEND_FOUND TRUE)
        set(BACKEND_TYPE "SDL2")
        set(BACKEND_LIBRARIES SDL2::SDL2)
        add_compile_definitions(IMGUI_BACKEND_SDL2)
        message(STATUS "midicci-keyboard-imgui: Using SDL2 backend")
    endif()
endif()

# Try GLFW
if(NOT BACKEND_FOUND)
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        set(BACKEND_FOUND TRUE)
        set(BACKEND_TYPE "GLFW")
        set(BACKEND_LIBRARIES glfw)
        add_compile_definitions(IMGUI_BACKEND_GLFW)
        message(STATUS "midicci-keyboard-imgui: Using GLFW backend")
    endif()
endif()

# Fallback: Use CPM to fetch GLFW if nothing found
if(NOT BACKEND_FOUND)
    message(STATUS "midicci-keyboard-imgui: No backend found, fetching GLFW via CPM")
    CPMAddPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS
            "GLFW_BUILD_EXAMPLES OFF"
            "GLFW_BUILD_TESTS OFF"
            "GLFW_BUILD_DOCS OFF"
            "GLFW_INSTALL OFF"
    )
    set(BACKEND_TYPE "GLFW")
    set(BACKEND_LIBRARIES glfw)
    add_compile_definitions(IMGUI_BACKEND_GLFW)
endif()

# Add ImGui via CPM
CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.91.5
    DOWNLOAD_ONLY YES
)

if(imgui_ADDED)
    # Create ImGui library with appropriate backend
    if(BACKEND_TYPE STREQUAL "SDL3")
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        )
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC ${BACKEND_LIBRARIES})
    elseif(BACKEND_TYPE STREQUAL "SDL2")
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
        )
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC ${BACKEND_LIBRARIES})
    else() # GLFW
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC ${BACKEND_LIBRARIES})
        find_package(OpenGL REQUIRED)
        target_link_libraries(imgui PUBLIC OpenGL::GL)
        if(UNIX AND NOT APPLE)
            target_link_libraries(imgui PUBLIC ${CMAKE_DL_LIBS})
        endif()
    endif()
endif()

# Add cmidi2 for UMP support
include(FetchContent)
FetchContent_Declare(cmidi2
    GIT_REPOSITORY https://github.com/atsushieno/cmidi2
    GIT_TAG        55fc50a
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cmidi2)

# Create the main executable
add_executable(midicci-keyboard-imgui
    src/main.cpp
)

target_link_libraries(midicci-keyboard-imgui
    PRIVATE
    imgui
    midicci-keyboard-core
    libremidi
    midicci
)

target_include_directories(midicci-keyboard-imgui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ump-keyboard/src
    ${cmidi2_SOURCE_DIR}
)
