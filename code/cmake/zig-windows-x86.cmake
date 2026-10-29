# Legacy GNU-ABI toolchain, retained for standalone experiments only.
# Original-game DLLs now reject this target; see clang-msvc-windows-x86.cmake.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES RA2_ZIG)
if(NOT DEFINED RA2_ZIG)
    message(FATAL_ERROR "Set RA2_ZIG to an absolute Zig executable path (tested with 0.14.1).")
endif()
set(CMAKE_C_COMPILER "${RA2_ZIG}")
set(CMAKE_C_COMPILER_ARG1 "cc -target x86-windows-gnu")
set(CMAKE_CXX_COMPILER "${RA2_ZIG}")
set(CMAKE_CXX_COMPILER_ARG1 "c++ -target x86-windows-gnu")
# Platform modules reset archive command templates after toolchain loading.
# Give them actual executables which dispatch Zig's subcommands instead.
foreach(tool ar ranlib)
    set(wrapper "${CMAKE_BINARY_DIR}/ra2-zig-${tool}")
    file(WRITE "${wrapper}" "#!/bin/sh\nexec \"${RA2_ZIG}\" ${tool} \"$@\"\n")
    file(CHMOD "${wrapper}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    string(TOUPPER "${tool}" upper_tool)
    set(CMAKE_${upper_tool} "${wrapper}")
endforeach()
