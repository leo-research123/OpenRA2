# Optional host adapter. No Godot configuration is evaluated by core-only builds.
# Stage the bridge's single shader source into the Godot resource tree. Run on
# every host build so deleted or stale generated resources are restored too.
add_custom_target(ra2_godot_shaders
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/godot/shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/bridge/shaders/map_draw.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/bridge/shaders/map_present.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/bridge/shaders/map_abuffer.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/bridge/shaders/map_abuffer_batch.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/godot/shaders"
    COMMENT "Preparing Godot resources from bridge shaders"
    VERBATIM
)
include(FetchContent)
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
)
FetchContent_MakeAvailable(godot_cpp)

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
    bridge/src/type_drawing_check.cpp
    bridge/src/type_drawing_packets.cpp
    bridge/src/lighting_batch.cpp
    bridge/src/ra2_core.cpp
    bridge/src/ra2_map_objects.cpp
    bridge/src/ra2_map_view.cpp
    bridge/src/map_input.cpp
    bridge/src/map_renderer.cpp
    bridge/src/map_resources.cpp
    bridge/src/ra2_ini.cpp
    bridge/src/register_types.cpp
    bridge/src/sprite_batch_renderer.cpp
    # Existing experimental scenes remain part of the extension.
    bridge/src/test_infantry.cpp
    bridge/src/test_infantry_graphics.cpp
    bridge/src/test_infantry_benchmark.cpp
    )
add_dependencies(ra2_godot ra2_godot_shaders)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experiments/test_ball.cpp")
    target_sources(ra2_godot PRIVATE bridge/src/test_ball.cpp experiments/test_ball.cpp)
    target_compile_definitions(ra2_godot PRIVATE RA2_HAS_TEST_BALL=1)
endif()
ra2_configure_cpp_target(ra2_godot)
if(MSVC)
    set_source_files_properties(bridge/src/ra2_map_objects.cpp bridge/src/ra2_map_view.cpp bridge/src/map_renderer.cpp bridge/src/map_resources.cpp bridge/src/lighting_batch.cpp PROPERTIES COMPILE_OPTIONS /EHsc)
else()
    set_source_files_properties(bridge/src/ra2_map_objects.cpp bridge/src/ra2_map_view.cpp bridge/src/map_renderer.cpp bridge/src/map_resources.cpp bridge/src/lighting_batch.cpp PROPERTIES COMPILE_OPTIONS -fexceptions)
endif()
target_include_directories(ra2_godot PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/bridge/include")
# Resource workers and test1 initialization catch core exceptions at the host
# boundary. Override godot-cpp's -fno-exceptions for these sources only.
if(MSVC)
    set_source_files_properties(bridge/src/type_drawing_check.cpp bridge/src/type_drawing_packets.cpp bridge/src/ra2_core.cpp bridge/src/test_infantry.cpp
        bridge/src/test_infantry_graphics.cpp bridge/src/test_infantry_benchmark.cpp PROPERTIES COMPILE_OPTIONS /EHsc)
else()
    set_source_files_properties(bridge/src/type_drawing_check.cpp bridge/src/type_drawing_packets.cpp bridge/src/ra2_core.cpp bridge/src/test_infantry.cpp
        bridge/src/test_infantry_graphics.cpp bridge/src/test_infantry_benchmark.cpp PROPERTIES COMPILE_OPTIONS -fexceptions)
endif()
set(ra2_godot_is_release "$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>")
target_link_libraries(ra2_godot PRIVATE ra2_core Threads::Threads
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
get_filename_component(ra2_output_root "${CMAKE_CURRENT_SOURCE_DIR}/../out" ABSOLUTE)
set(ra2_bin "${ra2_output_root}/$<IF:${ra2_godot_is_release},release,debug>/bin")
# Architectures share the bin directory and macOS exports flatten libraries
# into Frameworks, so keep the architecture in every library's filename.
set_target_properties(ra2_godot PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ra2_core.${ra2_arch}.$<IF:${ra2_godot_is_release},release,debug>"
    LIBRARY_OUTPUT_DIRECTORY "${ra2_bin}"
    RUNTIME_OUTPUT_DIRECTORY "${ra2_bin}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
