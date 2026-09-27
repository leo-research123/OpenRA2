include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
endif()
# Set before any project or dependency targets are created. Honor toolchain,
# command-line and existing cache selections; godot-cpp respects this cache entry.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    CACHE STRING "MSVC runtime shared by RA2 targets and their linked dependencies")

function(ra2_configure_cpp_target target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
        MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
    # Core code uses exceptions internally. They must be caught at host/ABI
    # entry boundaries, not propagated into Godot or the original executable.
    if(MSVC)
        target_compile_options(${target} PRIVATE /EHsc /utf-8)
    else()
        target_compile_options(${target} PRIVATE -fexceptions)
    endif()
endfunction()
