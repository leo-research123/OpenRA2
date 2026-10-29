include_guard(GLOBAL)

# Only this directory opts into automatic discovery. It contains standalone
# console commands; the original/core source sets remain explicitly maintained.
function(ra2_add_debug_command_objects target directory)
    file(GLOB command_sources CONFIGURE_DEPENDS "${directory}/*.cpp")
    if(NOT command_sources)
        message(FATAL_ERROR "No console command sources found in ${directory}")
    endif()
    foreach(source IN LISTS command_sources)
        get_filename_component(name "${source}" NAME_WE)
        if(NOT name MATCHES "^Debug[A-Za-z0-9]+Command$")
            message(FATAL_ERROR "Console command source must be named Debug<Name>Command.cpp: ${source}")
        endif()
    endforeach()
    add_library(${target} OBJECT ${command_sources})
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()
