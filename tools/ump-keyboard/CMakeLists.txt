# UMP Keyboard Tool
# A Qt-based MIDI keyboard application with MIDI-CI support

# Find Qt6 for GUI components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Add external dependencies
include(FetchContent)

# libremidi for MIDI I/O
FetchContent_Declare(libremidi
        GIT_REPOSITORY https://github.com/celtera/libremidi
        GIT_TAG        v5.2.0
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(libremidi)

# cmidi2 for UMP support
FetchContent_Declare(cmidi2
        GIT_REPOSITORY https://github.com/atsushieno/cmidi2
        GIT_TAG        0.7
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cmidi2)

# Add the source directory
add_subdirectory(src/)

# Enable testing and add tests subdirectory
enable_testing()
add_subdirectory(tests/)