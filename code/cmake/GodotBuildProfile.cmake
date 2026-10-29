include_guard(GLOBAL)

function(ra2_prepare_godot_api python script profile source_api output_api)
    get_filename_component(output_directory "${output_api}" DIRECTORY)
    file(MAKE_DIRECTORY "${output_directory}")
    set(temporary_api "${output_api}.tmp")
    execute_process(
        COMMAND "${python}" "${script}" "${profile}" "${source_api}" "${temporary_api}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result STREQUAL "0")
        file(REMOVE "${temporary_api}")
        message(FATAL_ERROR "Godot API trimming failed (${result}):\n${output}${error}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${temporary_api}" "${output_api}"
        COMMAND_ERROR_IS_FATAL ANY)
    file(REMOVE "${temporary_api}")
    # Input changes and a deleted output must trigger configuration again.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${profile}" "${source_api}" "${script}" "${output_api}")
endfunction()

function(ra2_add_godot_cpp source_directory binary_directory)
    # Declare typed cache entries before applying local overrides. This also
    # preserves overrides with the dependency's older CMP0126 policy.
    set(GODOTCPP_CUSTOM_API_FILE "" CACHE FILEPATH "Path to a custom GDExtension API JSON file")
    set(GODOTCPP_GDEXTENSION_DIR "gdextension" CACHE PATH "Path to the GDExtension interface and API")
    if(GODOTCPP_BUILD_PROFILE)
        find_package(Python3 3.4 REQUIRED COMPONENTS Interpreter)
        if(GODOTCPP_CUSTOM_API_FILE)
            set(source_api "${GODOTCPP_CUSTOM_API_FILE}")
        else()
            set(source_api "${GODOTCPP_GDEXTENSION_DIR}/extension_api.json")
        endif()
        get_filename_component(source_api "${source_api}" ABSOLUTE BASE_DIR "${source_directory}")
        get_filename_component(profile "${GODOTCPP_BUILD_PROFILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        set(stable_api "${binary_directory}/extension_api.json")
        ra2_prepare_godot_api("${Python3_EXECUTABLE}" "${source_directory}/build_profile.py"
            "${profile}" "${source_api}" "${stable_api}")
        # Keep user cache settings intact. The dependency consumes the prepared
        # API in this scope and must not rewrite it during configuration.
        set(GODOTCPP_CUSTOM_API_FILE "${stable_api}")
        set(GODOTCPP_BUILD_PROFILE "")
    endif()
    add_subdirectory("${source_directory}" "${binary_directory}")
endfunction()
