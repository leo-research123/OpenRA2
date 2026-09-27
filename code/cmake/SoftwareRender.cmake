# Optional CPU rasterization, not a platform-wide renderer interface.
# Basic resource/decoding core and Godot do not link this component implicitly.
include("${CMAKE_CURRENT_LIST_DIR}/SourceSets.cmake")
add_library(ra2_software_render STATIC
    ${ra2_convert_blitter_sources} ${ra2_software_render_sources})
add_library(ra2::software_render ALIAS ra2_software_render)
ra2_configure_cpp_target(ra2_software_render)
ra2_core_private_includes(ra2_software_render)
target_link_libraries(ra2_software_render PUBLIC ra2_core)
set_target_properties(ra2_software_render PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(ra2_software_render PRIVATE /fp:strict)
else()
    target_compile_options(ra2_software_render PRIVATE -fno-fast-math -ffp-contract=off)
endif()
