# Optional host adapter. No Godot configuration is evaluated by core-only builds.
include("${CMAKE_CURRENT_LIST_DIR}/TextEncoding.cmake")
if(NOT RA2_BUILD_RENDER_DEVICE)
    message(FATAL_ERROR "Godot requires RA2_BUILD_RENDER_DEVICE=ON")
endif()
option(RA2_BRIDGE_EXPERIMENTS "Compile bridge experiment classes" OFF)
set(RA2_GODOT_PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/godot" CACHE PATH "Godot resource destination")
get_filename_component(ra2_default_output "${CMAKE_CURRENT_SOURCE_DIR}/../out" ABSOLUTE)
set(RA2_GODOT_OUTPUT_ROOT "${ra2_default_output}" CACHE PATH "Published GDExtension output root")
file(MAKE_DIRECTORY "${RA2_GODOT_PROJECT}/third_party/icu")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/LICENSE.txt"
    "${RA2_GODOT_PROJECT}/third_party/icu/LICENSE.txt" COPYONLY)
add_custom_target(ra2_godot_maps
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${RA2_GODOT_PROJECT}/maps"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/data/maps/quick_test.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/data/maps/gi_10000.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/data/maps/rhino_2000.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/data/maps/vera-20000-30p-standby.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/data/maps/vera-20000-30p-combat.map"
        "${RA2_GODOT_PROJECT}/maps"
    COMMENT "Preparing the bundled test maps"
    VERBATIM
)
include(FetchContent)
include("${CMAKE_CURRENT_LIST_DIR}/GodotBuildProfile.cmake")
# Godot 4.4 API, compatible with newer Godot 4.x standard-precision builds.
set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json" CACHE FILEPATH "")
if(GODOTCPP_BUILD_PROFILE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${GODOTCPP_BUILD_PROFILE}")
endif()
set(GODOTCPP_ENABLE_TESTING OFF CACHE BOOL "")
set(GODOTCPP_MACOS_UNIVERSAL OFF CACHE BOOL "")
FetchContent_Declare(godot_cpp
    GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
    GIT_TAG 714c9e2c165db2dcb7e6ea57e62a04204d3cfbfa
    GIT_SUBMODULES ""
    # Populate first; prepare stable API input before configuring the dependency.
    SOURCE_SUBDIR ra2-populate-only
)
FetchContent_MakeAvailable(godot_cpp)
ra2_add_godot_cpp("${godot_cpp_SOURCE_DIR}" "${godot_cpp_BINARY_DIR}")

# The project selects the CRT before creating either the core or godot-cpp.
# A dependency must not silently change the ABI of the linked static libraries.
if(MSVC)
    get_target_property(ra2_core_crt ra2_core MSVC_RUNTIME_LIBRARY)
    foreach(godot_target godot-cpp::template_debug godot-cpp::template_release)
        get_target_property(godot_crt ${godot_target} MSVC_RUNTIME_LIBRARY)
        if(NOT "${godot_crt}" STREQUAL "${ra2_core_crt}")
            message(FATAL_ERROR "${godot_target} and ra2_core must use the same MSVC runtime.")
        endif()
    endforeach()
endif()

find_package(Threads REQUIRED)
add_library(ra2_godot SHARED
    bridge/src/ra2_core.cpp
    bridge/src/audio_output.cpp
    bridge/src/ra2_map_objects.cpp
    bridge/src/ra2_debug_commands.cpp
    bridge/src/ra2_map_view.cpp
    bridge/src/map_input.cpp
    bridge/src/godot_device.cpp
    bridge/src/godot_render_host.cpp
    bridge/src/ra2_ini.cpp
    bridge/src/register_types.cpp
    )
add_dependencies(ra2_godot ra2_render_shaders ra2_godot_maps)
# Host graphics experiments (test1, test-ball scenes under godot/scenes and
# godot/scripts/experiments). Production extensions neither compile nor register them.
if(RA2_BRIDGE_EXPERIMENTS)
    target_sources(ra2_godot PRIVATE
        bridge/src/sprite_batch_renderer.cpp
        bridge/src/test_infantry.cpp
        experiments/test_infantry_graphics.cpp
        bridge/src/test_infantry_benchmark.cpp)
    target_compile_definitions(ra2_godot PRIVATE RA2_BRIDGE_EXPERIMENTS=1)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experiments/test_ball.cpp")
        target_sources(ra2_godot PRIVATE bridge/src/test_ball.cpp experiments/test_ball.cpp)
        target_compile_definitions(ra2_godot PRIVATE RA2_HAS_TEST_BALL=1)
    endif()
endif()
ra2_configure_cpp_target(ra2_godot)
if(MSVC)
    set_source_files_properties(bridge/src/ra2_debug_commands.cpp bridge/src/ra2_map_objects.cpp bridge/src/ra2_map_view.cpp bridge/src/godot_render_host.cpp bridge/src/godot_device.cpp PROPERTIES COMPILE_OPTIONS /EHsc)
else()
    set_source_files_properties(bridge/src/ra2_debug_commands.cpp bridge/src/ra2_map_objects.cpp bridge/src/ra2_map_view.cpp bridge/src/godot_render_host.cpp bridge/src/godot_device.cpp PROPERTIES COMPILE_OPTIONS -fexceptions)
endif()
target_include_directories(ra2_godot PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/bridge/include" "${ra2_shader_stage}")
# Resource workers and INI queries catch core exceptions at the host boundary
# (test1 initialization does too, when the experiments are built). Override
# godot-cpp's -fno-exceptions for these sources only.
if(MSVC)
    set_source_files_properties(bridge/src/audio_output.cpp bridge/src/ra2_core.cpp bridge/src/ra2_ini.cpp bridge/src/test_infantry.cpp
        experiments/test_infantry_graphics.cpp bridge/src/test_infantry_benchmark.cpp PROPERTIES COMPILE_OPTIONS /EHsc)
else()
    set_source_files_properties(bridge/src/audio_output.cpp bridge/src/ra2_core.cpp bridge/src/ra2_ini.cpp bridge/src/test_infantry.cpp
        experiments/test_infantry_graphics.cpp bridge/src/test_infantry_benchmark.cpp PROPERTIES COMPILE_OPTIONS -fexceptions)
endif()
set(ra2_godot_is_release "$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>")
if(NOT RA2_BUILD_MOVIES)
    message(FATAL_ERROR "Godot campaign playback requires RA2_BUILD_MOVIES=ON")
endif()
target_link_libraries(ra2_godot PRIVATE ra2_render ra2_movies ra2_core ra2_text_encoding Threads::Threads
    "$<IF:${ra2_godot_is_release},godot-cpp::template_release,godot-cpp::template_debug>")
# Reuse the dependency's resolved architecture; its helper relies on directory-local variables.
get_target_property(ra2_arch godot-cpp::template_release GODOTCPP_ARCH)
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    list(LENGTH CMAKE_OSX_ARCHITECTURES ra2_osx_arch_count)
    if(ra2_osx_arch_count EQUAL 1 AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL ra2_arch)
        message(FATAL_ERROR
            "macOS compiler architecture '${CMAKE_OSX_ARCHITECTURES}' differs from godot-cpp '${ra2_arch}'. "
            "Use a fresh build directory with CMAKE_SYSTEM_NAME=Darwin, CMAKE_SYSTEM_PROCESSOR and "
            "CMAKE_OSX_ARCHITECTURES set to the target architecture at the first configure. "
            "Continuing would overwrite another architecture's GDExtension library.")
    endif()
endif()
if(NOT APPLE AND NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This desktop skeleton supports macOS, Windows and Linux.")
endif()
# Keep GDExtension binaries outside the Godot source tree. The configuration
# expression also prevents multi-config generators from adding a second suffix.
set(ra2_bin "${RA2_GODOT_OUTPUT_ROOT}/$<IF:${ra2_godot_is_release},release,debug>/bin")
# Architectures share the bin directory and macOS exports flatten libraries
# into Frameworks, so keep the architecture in every library's filename.
set_target_properties(ra2_godot PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ra2_core.${ra2_arch}.$<IF:${ra2_godot_is_release},release,debug>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/godot-library/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/godot-library/$<CONFIG>"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
# Link away from the active module. A failed link leaves the installed library
# intact; the rename publishes only a complete library with prepared resources.
add_custom_command(TARGET ra2_godot POST_BUILD
    COMMAND "${CMAKE_COMMAND}" "-DRA2_LIBRARY=$<TARGET_FILE:ra2_godot>"
        "-DRA2_DESTINATION=${ra2_bin}/$<TARGET_FILE_NAME:ra2_godot>"
        -P "${CMAKE_CURRENT_LIST_DIR}/GodotPublishLibrary.cmake"
    VERBATIM)
if(APPLE)
    add_executable(ra2_crash_reporter tools/crash_reporter.cpp)
    ra2_configure_cpp_target(ra2_crash_reporter)
    add_dependencies(ra2_godot ra2_crash_reporter)
    add_custom_command(TARGET ra2_godot POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:ra2_crash_reporter>" "${ra2_bin}/ra2_crash_reporter"
        VERBATIM)
endif()
