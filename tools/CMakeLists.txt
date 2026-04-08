
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
        GIT_TAG feature/winmidi_virtual
        EXCLUDE_FROM_ALL
)
midicci_fetchcontent_makeavailable_static(libremidi)

if(TARGET libremidi)
    # Ensure libremidi always builds against std::variant to avoid ABI mismatches when Boost headers are available only for our targets.
    get_target_property(_libremidi_type libremidi TYPE)
    if(_libremidi_type STREQUAL "INTERFACE_LIBRARY")
        target_compile_definitions(libremidi INTERFACE LIBREMIDI_NO_BOOST_VARIANT2)
    else()
        target_compile_definitions(libremidi PUBLIC LIBREMIDI_NO_BOOST_VARIANT2)
    endif()
endif()

add_subdirectory(tooling)
add_subdirectory(midicci-app)
