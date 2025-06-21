cmake_minimum_required(VERSION 3.18)

project(midicci-gui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5 components
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

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
    src/AppModel.cpp
)

set(HEADERS
    include/MainWindow.hpp
    include/InitiatorWidget.hpp
    include/ResponderWidget.hpp
    include/LogWidget.hpp
    include/SettingsWidget.hpp
    include/AppModel.hpp
)

add_executable(midicci-gui ${SOURCES} ${HEADERS})

target_include_directories(midicci-gui PRIVATE
    include
    ../tooling/include
    ../../include
)

target_link_libraries(midicci-gui
    Qt::Core
    Qt::Widgets
    midicci
    midicci-tooling
)

add_dependencies(midicci-gui midicci midicci-tooling)

if(WIN32)
    set_target_properties(midicci-gui PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
