# Host-independent render scheduling. No dependency on the simulation or a host SDK.
add_library(ra2_render STATIC
    render/src/frame_compiler.cpp
    render/src/pixel_batch.cpp
    render/src/raster_batch.cpp
    render/src/laser_batch.cpp)
# GPU orchestration is host-neutral; shaders are compiled only when a GPU host is requested.
option(RA2_BUILD_RENDER_DEVICE "Build device-backed render orchestration and SPIR-V" ${RA2_BUILD_GODOT})
if(RA2_BUILD_RENDER_DEVICE)
    include("${CMAKE_CURRENT_LIST_DIR}/RenderShaders.cmake")
    target_sources(ra2_render PRIVATE render/src/renderer.cpp render/src/renderer_api.cpp)
    target_include_directories(ra2_render PRIVATE "${ra2_shader_stage}")
    add_dependencies(ra2_render ra2_render_shaders)
endif()
add_library(ra2::render ALIAS ra2_render)
ra2_configure_cpp_target(ra2_render)
target_include_directories(ra2_render PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/render/include")
set_target_properties(ra2_render PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Presentation reads original resources synchronously and retains neutral render storage.
target_sources(ra2_core PRIVATE
    core/src/presentation/reference_frame.cpp
    core/src/presentation/frame_api.cpp
    core/src/presentation/frame_compiler.cpp
    core/src/presentation/type_drawing_packets.cpp
    core/src/presentation/lighting_batch.cpp)
target_include_directories(ra2_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/render/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/src/presentation")
target_link_libraries(ra2_core PUBLIC ra2_render)
