cmake_minimum_required(VERSION 3.18)

project(midicci-flutter-gui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(FLUTTER_EXECUTABLE flutter)

if(NOT FLUTTER_EXECUTABLE)
    message(STATUS "Flutter not found, skipping flutter_ci_tool")
    return()
endif()

message(STATUS "Flutter found at: ${FLUTTER_EXECUTABLE}")

set(WRAPPER_SOURCES
    native/ci_tool_wrapper.cpp
)

set(WRAPPER_HEADERS
    native/ci_tool_wrapper.h
)

add_library(midicci-flutter-wrapper SHARED ${WRAPPER_SOURCES} ${WRAPPER_HEADERS})

target_include_directories(midicci-flutter-wrapper PRIVATE
    native
    ../tooling/include
    ../../include
)

target_link_libraries(midicci-flutter-wrapper
    midicci
    midicci-tooling
)

add_dependencies(midicci-flutter-wrapper midicci midicci-tooling)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FLUTTER_BUILD_TARGET "linux")
    set(FLUTTER_OUTPUT_DIR "build/linux/x64/release/bundle")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(FLUTTER_BUILD_TARGET "macos")
    set(FLUTTER_OUTPUT_DIR "build/macos/Build/Products/Release")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(FLUTTER_BUILD_TARGET "windows")
    set(FLUTTER_OUTPUT_DIR "build/windows/runner/Release")
else()
    message(WARNING "Unsupported platform for Flutter desktop build: ${CMAKE_SYSTEM_NAME}")
    return()
endif()

add_custom_target(flutter-pub-get
    COMMAND ${FLUTTER_EXECUTABLE} pub get
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running flutter pub get"
)

add_custom_target(flutter-build
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:midicci-flutter-wrapper> ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMAND ${FLUTTER_EXECUTABLE} build ${FLUTTER_BUILD_TARGET} --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS flutter-pub-get midicci-flutter-wrapper
    COMMENT "Building Flutter app for ${FLUTTER_BUILD_TARGET}"
)

add_custom_target(flutter-run
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:midicci-flutter-wrapper> ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMAND ${FLUTTER_EXECUTABLE} run -d ${FLUTTER_BUILD_TARGET}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS flutter-pub-get midicci-flutter-wrapper
    COMMENT "Running Flutter app on ${FLUTTER_BUILD_TARGET}"
)

# Custom target to copy all required libraries to Flutter app bundle
add_custom_target(flutter-bundle-libs
    COMMAND ${CMAKE_COMMAND} -E make_directory \"${CMAKE_CURRENT_SOURCE_DIR}/build/${FLUTTER_BUILD_TARGET}/Build/Products/Debug/midicci_flutter_gui.app/Contents/Frameworks/\"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:midicci-flutter-wrapper> \"${CMAKE_CURRENT_SOURCE_DIR}/build/${FLUTTER_BUILD_TARGET}/Build/Products/Debug/midicci_flutter_gui.app/Contents/Frameworks/\"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:midicci> \"${CMAKE_CURRENT_SOURCE_DIR}/build/${FLUTTER_BUILD_TARGET}/Build/Products/Debug/midicci_flutter_gui.app/Contents/Frameworks/\"
    DEPENDS midicci-flutter-wrapper midicci
    COMMENT "Copying native libraries to Flutter app bundle"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FLUTTER_OUTPUT_DIR}")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${FLUTTER_OUTPUT_DIR}/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}/midicci-flutter-gui
        USE_SOURCE_PERMISSIONS
    )
endif()

install(TARGETS midicci-flutter-wrapper
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
