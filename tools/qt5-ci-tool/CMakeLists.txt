cmake_minimum_required(VERSION 3.20)

project(qt5-midi-ci-gui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Widgets)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/InitiatorWidget.cpp
    src/ResponderWidget.cpp
    src/LogWidget.cpp
    src/SettingsWidget.cpp
)

set(HEADERS
    include/MainWindow.hpp
    include/InitiatorWidget.hpp
    include/ResponderWidget.hpp
    include/LogWidget.hpp
    include/SettingsWidget.hpp
)

add_executable(qt5-midi-ci-gui ${SOURCES} ${HEADERS})

target_include_directories(qt5-midi-ci-gui PRIVATE
    include
    ../ci-tool/include
    ../../include
)

target_link_libraries(qt5-midi-ci-gui
    Qt::Core
    Qt::Widgets
    midi-ci-tool-lib
)

if(WIN32)
    set_target_properties(qt5-midi-ci-gui PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
