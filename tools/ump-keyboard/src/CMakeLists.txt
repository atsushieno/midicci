qt6_standard_project_setup()

add_executable(ump-keyboard
    main.cpp
    keyboard_controller.cpp
    keyboard_widget.cpp
    keyboard_widget.h
    virtualized_control_list.cpp
    virtualized_control_list.h
    midi_ci_manager.cpp
    midi_ci_manager.h
)

target_link_libraries(ump-keyboard 
    Qt6::Core
    Qt6::Widgets
    libremidi
    midicci
)

target_include_directories(ump-keyboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${cmidi2_SOURCE_DIR}
)