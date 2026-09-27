cmake_minimum_required(VERSION 3.24)
project(ra2_msvc_toolchain_smoke LANGUAGES C CXX)
# This target intentionally uses NO target headers, libraries, CRT or entry point.
# It checks only compiler, archiver, linker, machine type and calling convention.
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")
add_library(ra2_msvc_toolchain_smoke SHARED smoke.cpp)
target_compile_options(ra2_msvc_toolchain_smoke PRIVATE /GS- /GR-)
target_link_options(ra2_msvc_toolchain_smoke PRIVATE /nodefaultlib /noentry)
set_target_properties(ra2_msvc_toolchain_smoke PROPERTIES PREFIX "")
