include("${CMAKE_CURRENT_LIST_DIR}/GoogleTestSupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/SourceSets.cmake")

# P1 public-class contract tests; no private implementation headers.

# Keep dependencies and private-header access explicit beside each test.
function(ra2_add_native_test target name)
    add_executable(${target} ${ARGN})
    ra2_configure_cpp_target(${target})
    ra2_use_googletest(${target})
    add_test(NAME ${name} COMMAND ${target})
endfunction()

ra2_add_native_test(ra2_abstract_class_tests abstract_classes tests/abstract_class_tests.cpp)
ra2_add_native_test(ra2_swizzle_tests swizzle tests/swizzle_tests.cpp
    core/src/yrpp/SwizzleManagerClass.cpp)
# COM/vector contracts can run independently of the world implementation.
target_include_directories(ra2_swizzle_tests PRIVATE core/include)
target_compile_definitions(ra2_swizzle_tests PRIVATE
    RA2_SWIZZLE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/swizzle_reference.txt")
target_link_libraries(ra2_abstract_class_tests PRIVATE ra2_core)
target_compile_definitions(ra2_abstract_class_tests PRIVATE
    RA2_ABSTRACT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/abstract_reference.txt")

ra2_add_native_test(ra2_rules_tests rules tests/rules_tests.cpp)
ra2_add_native_test(ra2_weapon_type_tests weapon_type tests/weapon_type_tests.cpp)
ra2_add_native_test(ra2_weapon_stream_tests weapon_stream tests/weapon_stream_tests.cpp)
ra2_add_native_test(ra2_warhead_type_tests warhead_type tests/warhead_type_tests.cpp)
ra2_add_native_test(ra2_bullet_type_tests bullet_type tests/bullet_type_tests.cpp)
target_link_libraries(ra2_bullet_type_tests PRIVATE ra2_core)
target_compile_definitions(ra2_bullet_type_tests PRIVATE
    RA2_BULLET_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bullet_type_reference.txt")
ra2_add_native_test(ra2_logic_layer_tests logic_layer tests/logic_layer_tests.cpp)
ra2_add_native_test(ra2_mission_orders_tests mission_orders tests/mission_orders_tests.cpp)
ra2_add_native_test(ra2_object_mission_update_tests object_mission_update tests/object_mission_update_tests.cpp)
ra2_add_native_test(ra2_projectile_diagnostics_tests projectile_diagnostics tests/projectile_diagnostics_tests.cpp)
ra2_add_native_test(ra2_voxel_ramp_tests voxel_ramp tests/voxel_ramp_tests.cpp)
target_link_libraries(ra2_voxel_ramp_tests PRIVATE ra2_core)
target_compile_definitions(ra2_voxel_ramp_tests PRIVATE
    RA2_VOXEL_RAMP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/voxel_ramp_reference.txt"
    RA2_VOXEL_RAMP_USAGE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/voxel_ramp_usage_reference.txt")
# Explicit access to the internal slope-transition caller; math/locomotors are original classes.
ra2_core_private_includes(ra2_voxel_ramp_tests)
target_link_libraries(ra2_projectile_diagnostics_tests PRIVATE ra2_core)
# Explicit access to diagnostic hooks; no gameplay implementation is replaced.
ra2_core_private_includes(ra2_projectile_diagnostics_tests)
ra2_add_native_test(ra2_mirage_disguise_tests mirage_disguise tests/mirage_disguise_tests.cpp)
target_link_libraries(ra2_mirage_disguise_tests PRIVATE ra2_core)
# Scoped session-mode dependency for original house visibility queries.
ra2_core_private_includes(ra2_mirage_disguise_tests)
target_compile_definitions(ra2_mirage_disguise_tests PRIVATE
    RA2_MIRAGE_DISGUISE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mirage_disguise_reference.txt"
    RA2_UNIT_TARGETING_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_targeting_reference.txt")
ra2_add_native_test(ra2_flaming_guy_tests flaming_guy tests/flaming_guy_tests.cpp)
target_link_libraries(ra2_flaming_guy_tests PRIVATE ra2_core)
target_compile_definitions(ra2_flaming_guy_tests PRIVATE
    RA2_FLAMING_GUY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/flaming_guy_reference.txt")
target_link_libraries(ra2_object_mission_update_tests PRIVATE ra2_core)
target_compile_definitions(ra2_object_mission_update_tests PRIVATE
    RA2_OBJECT_MISSION_UPDATE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/object_mission_update_reference.txt")
target_link_libraries(ra2_mission_orders_tests PRIVATE ra2_core)
target_compile_definitions(ra2_mission_orders_tests PRIVATE
    RA2_MISSION_ORDERS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mission_orders_reference.txt")
ra2_add_native_test(ra2_infantry_display_tests infantry_display tests/infantry_display_tests.cpp)
ra2_add_native_test(ra2_infantry_frame_decision_tests infantry_frame_decisions tests/infantry_frame_decision_tests.cpp)
target_link_libraries(ra2_infantry_frame_decision_tests PRIVATE ra2_core)
# Explicit access to the scoped session-mode dependency used by House queries.
ra2_core_private_includes(ra2_infantry_frame_decision_tests)
target_compile_definitions(ra2_infantry_frame_decision_tests PRIVATE
    RA2_INFANTRY_FEAR_FIRE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_fear_fire_reference.txt")
ra2_add_native_test(ra2_infantry_passability_tests infantry_passability tests/infantry_passability_tests.cpp)
target_link_libraries(ra2_infantry_passability_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_infantry_passability_tests)
target_compile_definitions(ra2_infantry_passability_tests PRIVATE
    RA2_INFANTRY_PASSABILITY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_passability_reference.txt"
    RA2_ASTAR_COST_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/astar_cost_reference.txt"
    RA2_ASTAR_REGULAR_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/astar_regular_reference.txt"
    RA2_MAP_REGION_THREAT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_region_threat_reference.txt"
    RA2_ASTAR_HIERARCHY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/astar_hierarchy_reference.txt"
    RA2_MAP_ZONES_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_zones_reference.txt"
    RA2_CELL_PASSABILITY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cell_passability_reference.txt")
target_link_libraries(ra2_infantry_display_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_particle_tests particles tests/particle_tests.cpp)
target_link_libraries(ra2_particle_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_particle_tests)
target_compile_definitions(ra2_particle_tests PRIVATE RA2_SPARK_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/spark_reference.txt")
target_compile_definitions(ra2_particle_tests PRIVATE RA2_SMOKE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/smoke_reference.txt")
target_compile_definitions(ra2_infantry_display_tests PRIVATE
    RA2_MIRAGE_DISGUISE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mirage_disguise_reference.txt"
    RA2_FOOT_DRAWING_DEPTH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/foot_drawing_depth_reference.txt"
    RA2_UNIT_GUNNER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_gunner_reference.txt"
    RA2_PRODUCTION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/production_reference.txt"
    RA2_HARVEST_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/harvest_reference.txt"
    RA2_HARVEST_TIMING_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/harvest_timing_reference.txt"
    RA2_HARVEST_DEPLETION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/harvest_depletion_reference.txt"
    RA2_UNIT_FIRE_COORDINATES_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_fire_coordinates_reference.txt"
    RA2_BUILDING_FIRE_COORDINATES_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_fire_coordinates_reference.txt"
    RA2_BUILDING_FIRE_EXTENDED_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_fire_extended_reference.txt"
    RA2_UNIT_FIRING_FACING_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_firing_facing_reference.txt"
    RA2_MIND_CONTROL_DISPATCH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mind_control_dispatch_reference.txt"
    RA2_MIND_CONTROL_PIXELS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mind_control_line_pixels.txt"
    RA2_MIND_CONTROL_LINKS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mind_control_links_reference.txt"
    RA2_CAPTURE_MANAGER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/capture_manager_reference.txt"
    RA2_PROJECTILE_PITCH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_pitch_reference.txt"
    RA2_BULLET_DRAWING_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bullet_drawing_reference.txt"
    RA2_ACTION_LINE_PIXELS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/action_line_pixels_reference.txt"
    RA2_ACTION_LINE_BODY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/action_line_body_reference.txt"
    RA2_ACTION_LINE_CALL_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/action_line_call_reference.txt"
    RA2_DRIVE_REFERENCE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/drive_reference.txt"
    RA2_BUILDING_ATTACK_RANGE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_attack_range_reference.txt"
    RA2_BUILDING_ATTACK_APPROACH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_attack_approach_reference.txt"
    RA2_INFANTRY_BUILDING_FIRE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_building_fire_reference.txt"
    RA2_INFANTRY_FRAME_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_frame_reference.txt"
    RA2_INFANTRY_ACTION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_action_reference.txt"
    RA2_INFANTRY_EXPIRATION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_expiration_reference.txt"
    RA2_WALK_LOCOMOTION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/walk_locomotion_reference.txt"
    RA2_WALK_PROCESS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/walk_process_reference.txt"
    RA2_WALK_RESERVATION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/walk_reservation_reference.txt"
    RA2_FOOT_DESTINATION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/foot_destination_reference.txt"
    RA2_INFANTRY_SPEED_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_speed_reference.txt"
    RA2_INFANTRY_FRAME_UPDATE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_frame_update_reference.txt"
    RA2_INFANTRY_IDLE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_idle_reference.txt"
    RA2_TECHNO_DISCOVERY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/techno_discovery_reference.txt"
    RA2_FOOT_ARRIVAL_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/foot_arrival_reference.txt"
    RA2_INFANTRY_ARRIVAL_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_arrival_reference.txt"
    RA2_VISIBILITY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/visibility_reference.txt"
    RA2_PARADROP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/paradrop_reference.txt"
    RA2_GATTLING_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/gattling_reference.txt"
    RA2_WEAPON_SELECTION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_selection_reference.txt"
    RA2_FOOT_REACH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/foot_reach_reference.txt"
    RA2_INFANTRY_OCCUPATION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/infantry_occupation_reference.txt")
# Explicit presentation tests: actual map-world sprite packets and registries.
# CampaignPresets also exercises scenario_loading.hpp's shared reader order and
# native type-loading boundary; these internal helpers are not public YRpp APIs.
ra2_core_private_includes(ra2_infantry_display_tests)
target_link_libraries(ra2_logic_layer_tests PRIVATE ra2_core)
# Original game-loop ordering is tested through its private host clock scope.
ra2_core_private_includes(ra2_logic_layer_tests)
target_compile_definitions(ra2_logic_layer_tests PRIVATE
    RA2_LAYER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/logic_layer_reference.txt")
target_link_libraries(ra2_warhead_type_tests PRIVATE ra2_core)
target_compile_definitions(ra2_warhead_type_tests PRIVATE
    RA2_WARHEAD_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/warhead_type_reference.txt")
target_link_libraries(ra2_weapon_stream_tests PRIVATE ra2_core)
target_compile_definitions(ra2_weapon_stream_tests PRIVATE
    RA2_WEAPON_STREAM_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_stream_reference.txt")
target_link_libraries(ra2_weapon_type_tests PRIVATE ra2_core)
target_compile_definitions(ra2_weapon_type_tests PRIVATE
    RA2_WEAPON_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_type_reference.txt")
target_link_libraries(ra2_rules_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_rules_readers_tests rules_readers tests/rules_readers_tests.cpp)
target_link_libraries(ra2_rules_readers_tests PRIVATE ra2_core)

ra2_add_native_test(ra2_scenario_tests scenario tests/scenario_tests.cpp)
target_link_libraries(ra2_scenario_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_scenario_start_tests scenario_start tests/scenario_start_tests.cpp)
target_link_libraries(ra2_scenario_start_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_scenario_destruction_tests scenario_destruction tests/scenario_destruction_tests.cpp)
target_link_libraries(ra2_scenario_destruction_tests PRIVATE ra2_core)
target_compile_definitions(ra2_scenario_tests PRIVATE
    RA2_SCENARIO_RANDOM_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scenario_random_reference.txt")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experiments/test_ball.cpp")
    ra2_add_native_test(ra2_test_ball_tests test_ball tests/test_ball_tests.cpp experiments/test_ball.cpp)
    ra2_core_private_includes(ra2_test_ball_tests)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experiments/tile_half_invert.cpp")
    ra2_add_native_test(ra2_tile_invert_tests tile_invert tests/tile_invert_tests.cpp experiments/tile_half_invert.cpp)
    target_include_directories(ra2_tile_invert_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/core/include")
endif()

ra2_add_native_test(ra2_raw_file_tests raw_file tests/raw_file_tests.cpp)
target_link_libraries(ra2_raw_file_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_raw_file_tests)
set_tests_properties(raw_file PROPERTIES TIMEOUT 15)

ra2_add_native_test(ra2_vox_tests vox tests/vox_tests.cpp)
target_link_libraries(ra2_vox_tests PRIVATE ra2_core)

ra2_add_native_test(ra2_audio_tests audio tests/audio_tests.cpp)
target_link_libraries(ra2_audio_tests PRIVATE ra2_core)
set_tests_properties(audio PROPERTIES TIMEOUT 15)
ra2_add_native_test(ra2_audio_backend_tests audio_backend tests/audio_backend_tests.cpp)
target_link_libraries(ra2_audio_backend_tests PRIVATE ra2_core)
# Only yrpp/AudioHelpers.hpp is used from core/src, to exercise the two opaque
# original dependency boundaries. Host consumers use api/audio_backend.hpp.
ra2_core_private_includes(ra2_audio_backend_tests)
set_tests_properties(audio_backend PROPERTIES TIMEOUT 15)

ra2_add_native_test(ra2_video_backend_tests video_backend tests/video_backend_tests.cpp)
target_link_libraries(ra2_video_backend_tests PRIVATE ra2_core)
set_tests_properties(video_backend PROPERTIES TIMEOUT 15)

ra2_add_native_test(ra2_ini_tests ini tests/ini_tests.cpp)
target_link_libraries(ra2_ini_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_ini_tests)

# All newly completed INI interfaces, through public headers only.
ra2_add_native_test(ra2_ini_value_tests ini_values tests/ini_value_tests.cpp)
target_link_libraries(ra2_ini_value_tests PRIVATE ra2_core)

ra2_add_native_test(ra2_resource_tests resource_system tests/resource_tests.cpp)
target_link_libraries(ra2_resource_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_resource_tests)
target_compile_definitions(ra2_resource_tests PRIVATE
    RA2_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")

ra2_add_native_test(ra2_yrpp_value_tests yrpp_values tests/yrpp_value_tests.cpp)
target_link_libraries(ra2_yrpp_value_tests PRIVATE ra2_core)

ra2_add_native_test(ra2_yrpp_header_contract_tests yrpp_header_contracts tests/yrpp_header_contract_tests.cpp)
target_link_libraries(ra2_yrpp_header_contract_tests PRIVATE ra2_core)
target_compile_definitions(ra2_yrpp_header_contract_tests PRIVATE
    RA2_CRC_SCALAR_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/crc_scalar_reference.txt")

ra2_add_native_test(ra2_yrpp_memory_tests yrpp_memory tests/yrpp_memory_tests.cpp)
target_link_libraries(ra2_yrpp_memory_tests PRIVATE ra2_core)

add_executable(ra2_yrpp_memory_failure_tests
    tests/yrpp_memory_failure_tests.cpp ${ra2_core_memory_sources}
    ${ra2_filesystem_name_sources} core/src/yrpp/YRAllocator.cpp core/src/yrpp/Surface.cpp)
ra2_configure_cpp_target(ra2_yrpp_memory_failure_tests)
ra2_use_googletest(ra2_yrpp_memory_failure_tests EXTRA_ARGS --contracts)
target_compile_definitions(ra2_yrpp_memory_failure_tests PRIVATE RA2_MEMORY_TESTING)
add_test(NAME yrpp_memory_faults COMMAND ra2_yrpp_memory_failure_tests --contracts)
ra2_core_private_includes(ra2_yrpp_memory_failure_tests)

ra2_add_native_test(ra2_yrpp_memory_recovery_tests yrpp_memory_recovery
    tests/yrpp_memory_recovery_tests.cpp ${ra2_core_memory_sources}
    ${ra2_filesystem_name_sources} core/src/yrpp/FileClass.cpp)
ra2_core_private_includes(ra2_yrpp_memory_recovery_tests)
target_compile_definitions(ra2_yrpp_memory_recovery_tests PRIVATE RA2_MEMORY_TESTING)
if(WIN32)
    set(ra2_memory_failure_exit 805306405) # 0x30000000 | 37
else()
    set(ra2_memory_failure_exit 37) # POSIX preserves the low eight exit-status bits.
endif()
add_test(NAME yrpp_memory_failure COMMAND "${CMAKE_COMMAND}"
    "-DPROBE=$<TARGET_FILE:ra2_yrpp_memory_failure_tests>"
    "-DEXPECTED=${ra2_memory_failure_exit}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_memory_failure.cmake")
add_test(NAME yrpp_memory_recovery_failure COMMAND "${CMAKE_COMMAND}"
    "-DPROBE=$<TARGET_FILE:ra2_yrpp_memory_recovery_tests>"
    "-DPROBE_ARGUMENT=--checked-failure" "-DEXPECTED=${ra2_memory_failure_exit}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_memory_failure.cmake")

ra2_add_native_test(ra2_bootstrap_contract_tests bootstrap_contract
    tests/bootstrap_contract_tests.cpp core/src/yrpp/MixFileClassBootstrap.cpp)
ra2_core_private_includes(ra2_bootstrap_contract_tests)
# Object construction/destruction with production Surface/color operations.
# Only Alpha allocation accounting/resource inputs are unit-test fixtures.
ra2_add_native_test(ra2_convert_lifecycle_tests convert_lifecycle
    tests/convert_lifecycle_tests.cpp
    core/src/yrpp/Drawing.cpp
    core/src/yrpp/DrawingColorState.cpp
    core/src/yrpp/DrawingBuffers.cpp
    core/src/yrpp/Surface.cpp
    core/src/yrpp/YRAllocator.cpp
    ${ra2_core_memory_sources}
    ${ra2_palette_sources}
    ${ra2_convert_blitter_sources}
    core/src/yrpp/BasicStructures.cpp)
ra2_core_private_includes(ra2_convert_lifecycle_tests)
target_compile_definitions(ra2_convert_lifecycle_tests PRIVATE RA2_MEMORY_TESTING)
ra2_add_native_test(ra2_palette_table_tests palette_tables tests/palette_table_tests.cpp)
target_link_libraries(ra2_palette_table_tests PRIVATE ra2_core)
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_library(ra2_palette_probe SHARED tests/x86/palette_probe.cpp)
    ra2_configure_cpp_target(ra2_palette_probe)
    target_link_libraries(ra2_palette_probe PRIVATE ra2_core)
    # The differential emulator also maps the original-image DLL at 10000000.
    target_link_options(ra2_palette_probe PRIVATE /BASE:0x20000000)
endif()

add_library(ra2_public_blitter_tests OBJECT tests/public_blitter_tests.cpp)
ra2_configure_cpp_target(ra2_public_blitter_tests)
# An object-library source does not inherit includes from the final executable.
target_link_libraries(ra2_public_blitter_tests PRIVATE ra2_test_support)
target_include_directories(ra2_public_blitter_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core/include")
target_sources(ra2_convert_lifecycle_tests PRIVATE $<TARGET_OBJECTS:ra2_public_blitter_tests>)

ra2_add_native_test(ra2_public_core_tests public_core tests/public_core_tests.cpp)
target_link_libraries(ra2_public_core_tests PRIVATE ra2_core)

ra2_add_native_test(ra2_file_chain_tests file_chain tests/file_chain_tests.cpp)
target_link_libraries(ra2_file_chain_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_file_chain_tests)
set_tests_properties(file_chain PROPERTIES TIMEOUT 15)

ra2_add_native_test(ra2_shp_tests shp tests/shp_tests.cpp)
target_link_libraries(ra2_shp_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_shp_tests)

ra2_add_native_test(ra2_voxel_format_tests voxel_formats tests/voxel_format_tests.cpp)
target_link_libraries(ra2_voxel_format_tests PRIVATE ra2_core)
# Tests use only images/voxel_palette.hpp and filesystem resource setup internally.
ra2_core_private_includes(ra2_voxel_format_tests)

# Public API test: real core Surface and MemoryBuffer, no image-service doubles.
ra2_add_native_test(ra2_surface_tests surface tests/surface_tests.cpp)
target_link_libraries(ra2_surface_tests PRIVATE ra2_core)

ra2_add_native_test(ra2_planning_validation_tests planning_validation tests/planning_validation_tests.cpp)
target_link_libraries(ra2_planning_validation_tests PRIVATE ra2_core)
# Original planner feedback uses the resource-owned GAME.FNT and message list.
ra2_core_private_includes(ra2_planning_validation_tests)

ra2_add_native_test(ra2_messages_tests messages tests/messages_tests.cpp)
target_link_libraries(ra2_messages_tests PRIVATE ra2_core)
# Test original message construction under the UI font/resource and session scopes.
ra2_core_private_includes(ra2_messages_tests)

ra2_add_native_test(ra2_bitfont_tests bitfont tests/bitfont_tests.cpp)
target_link_libraries(ra2_bitfont_tests PRIVATE ra2_core)
target_compile_definitions(ra2_bitfont_tests PRIVATE
    RA2_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")

# Link every base-core object: static archive creation alone is not an ABI check.
ra2_add_native_test(ra2_core_full_link core_full_link tests/core_full_link.cpp)
target_link_libraries(ra2_core_full_link PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,ra2_core>")

# Public lifecycle contract; no private image or filesystem includes.
ra2_add_native_test(ra2_resource_shutdown_tests resource_shutdown tests/resource_shutdown_tests.cpp)
target_link_libraries(ra2_resource_shutdown_tests PRIVATE ra2_core)

if(TARGET ra2_software_render)
    ra2_add_native_test(ra2_radar_cursor_tests radar_cursor tests/radar_cursor_tests.cpp)
    target_link_libraries(ra2_radar_cursor_tests PRIVATE ra2_software_render)
    ra2_add_native_test(ra2_beacon_drawing_tests beacon_drawing tests/beacon_drawing_tests.cpp)
    target_link_libraries(ra2_beacon_drawing_tests PRIVATE ra2_software_render)
    # Original UI scope is internal; backend and resource handles use public APIs.
    ra2_core_private_includes(ra2_beacon_drawing_tests)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experiments/tile_half_invert.cpp" AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experiments/tile_half_invert.hpp")
    ra2_add_native_test(ra2_tmp_resource_tests tmp_resources tests/tmp_resource_tests.cpp experiments/tile_half_invert.cpp)
    target_include_directories(ra2_tmp_resource_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(ra2_tmp_resource_tests PRIVATE ra2_software_render)
    else()
        message(WARNING "tmp_resources cannot build: uploaded sources omit experiments/tile_half_invert.cpp/.hpp; retaining a failing test")
        add_test(NAME tmp_resources COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MissingTmpExperiment.cmake")
    endif()
    if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_library(ra2_tmp_probe SHARED tests/x86/tmp_probe.cpp)
        ra2_configure_cpp_target(ra2_tmp_probe)
        target_link_libraries(ra2_tmp_probe PRIVATE ra2_core)
    endif()
    ra2_add_native_test(ra2_software_render_tests shp_draw tests/software_render_tests.cpp)
    target_link_libraries(ra2_software_render_tests PRIVATE ra2_software_render)
    ra2_add_native_test(ra2_software_render_link software_render_link tests/software_render_link.cpp)
    target_link_libraries(ra2_software_render_link PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,ra2_software_render>")
    ra2_add_native_test(ra2_pcx_read_tests pcx_read tests/pcx_read_tests.cpp)
    target_link_libraries(ra2_pcx_read_tests PRIVATE ra2_software_render)
    ra2_add_native_test(ra2_image_cache_tests image_cache tests/image_cache_tests.cpp)
    target_link_libraries(ra2_image_cache_tests PRIVATE ra2_software_render)
endif()

# Original class resource/geometry methods, using public headers and real core I/O.
ra2_add_native_test(ra2_map_resource_tests map_resources tests/map_resource_tests.cpp)
target_link_libraries(ra2_map_resource_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_map_projection_tests map_projection tests/map_projection_tests.cpp)
ra2_add_native_test(ra2_map_camera_tests map_camera tests/map_camera_tests.cpp)
ra2_add_native_test(ra2_map_radar_tests map_radar tests/map_radar_tests.cpp)
ra2_add_native_test(ra2_radar_event_tests radar_event tests/radar_event_tests.cpp)
target_link_libraries(ra2_radar_event_tests PRIVATE ra2_core)
# Exercises original Draw through the borrowed generic frame scope; no host UI.
target_include_directories(ra2_radar_event_tests PRIVATE core/src)
target_compile_definitions(ra2_radar_event_tests PRIVATE
    RA2_RADAR_EVENT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/radar_event_reference.txt")
ra2_add_native_test(ra2_game_ui_input_tests game_ui_input tests/game_ui_input_tests.cpp)
target_link_libraries(ra2_game_ui_input_tests PRIVATE ra2_core)
# Fixture constructs real cells through MapViewHandle; no host substitute state.
ra2_core_private_includes(ra2_game_ui_input_tests)
ra2_add_native_test(ra2_game_ui_layout_tests game_ui_layout tests/game_ui_layout_tests.cpp)
ra2_add_native_test(ra2_power_tests power tests/power_tests.cpp)
ra2_add_native_test(ra2_map_configuration_tests map_configuration tests/map_configuration_tests.cpp)
ra2_add_native_test(ra2_building_placement_tests building_placement tests/building_placement_tests.cpp)
target_link_libraries(ra2_building_placement_tests PRIVATE ra2_core)
# Explicit internal access: scoped MapView/session and synthetic resource setup.
ra2_core_private_includes(ra2_building_placement_tests)
target_compile_definitions(ra2_building_placement_tests PRIVATE
    RA2_BUILDING_PLACEMENT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_placement_reference.txt")
target_link_libraries(ra2_map_configuration_tests PRIVATE ra2_core)
# Inspect retained source INIs and passive definitions; no host/UI substitute.
ra2_core_private_includes(ra2_map_configuration_tests)
target_link_libraries(ra2_power_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_power_tests)
target_compile_definitions(ra2_power_tests PRIVATE
    RA2_POWER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/power_reference.txt")
target_link_libraries(ra2_game_ui_layout_tests PRIVATE ra2_core)
target_compile_definitions(ra2_game_ui_layout_tests PRIVATE
    RA2_UI_LAYOUT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_ui_layout_reference.txt")
add_executable(ra2_game_ui_assets_reference tests/game_ui_assets_reference.cpp)
ra2_configure_cpp_target(ra2_game_ui_assets_reference)
target_link_libraries(ra2_game_ui_assets_reference PRIVATE ra2_core)
target_link_libraries(ra2_map_radar_tests PRIVATE ra2_core)
# Explicit internal access: borrowed drawing/input and Scenario scopes, plus
# canonical native foundation source storage for original fixed-span copies.
ra2_core_private_includes(ra2_map_radar_tests)
target_compile_definitions(ra2_map_radar_tests PRIVATE
    RA2_RADAR_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_radar_reference.txt"
    RA2_RADAR_WORLD="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_radar_world_reference.txt"
    RA2_RADAR_PIXELS="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_radar_pixels.bin")
target_link_libraries(ra2_map_camera_tests PRIVATE ra2_core)
target_compile_definitions(ra2_map_camera_tests PRIVATE
    RA2_CAMERA_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_camera_reference.txt")
target_link_libraries(ra2_map_projection_tests PRIVATE ra2_core)
# Tests inspect only the internal map_runtime projection scope, never host code.
ra2_core_private_includes(ra2_map_projection_tests)
target_compile_definitions(ra2_map_projection_tests PRIVATE
    RA2_MAP_PROJECTION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_projection_reference.txt")
# Include-order and static assertions only; no empty runtime process.
add_library(ra2_map_headers_compile OBJECT tests/map_headers_compile.cpp)
ra2_configure_cpp_target(ra2_map_headers_compile)
target_include_directories(ra2_map_headers_compile PRIVATE core/include)
ra2_add_native_test(ra2_cell_lifecycle_tests cell_lifecycle tests/cell_lifecycle_tests.cpp)
ra2_add_native_test(ra2_cell_overlay_tests cell_overlay tests/cell_overlay_tests.cpp)
ra2_add_native_test(ra2_sprite_drawing_tests sprite_drawing tests/sprite_drawing_tests.cpp)
ra2_add_native_test(ra2_techno_drawing_tests techno_drawing tests/techno_drawing_tests.cpp)
target_link_libraries(ra2_techno_drawing_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_techno_drawing_tests)
target_compile_definitions(ra2_techno_drawing_tests PRIVATE
    RA2_TECHNO_DRAW_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/techno_draw_reference.txt")
target_link_libraries(ra2_sprite_drawing_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_sprite_drawing_tests)
target_compile_definitions(ra2_sprite_drawing_tests PRIVATE
    RA2_SPRITE_DRAW_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sprite_draw_reference.txt")
target_link_libraries(ra2_cell_overlay_tests PRIVATE ra2_core)
# Explicit test-only access: original Cell overlay submission scope.
ra2_core_private_includes(ra2_cell_overlay_tests)
target_compile_definitions(ra2_cell_overlay_tests PRIVATE
    RA2_CELL_OVERLAY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cell_overlay_reference.txt")
target_link_libraries(ra2_cell_lifecycle_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_cell_lifecycle_tests)
ra2_add_native_test(ra2_map_lifecycle_tests map_lifecycle tests/map_lifecycle_tests.cpp)
target_link_libraries(ra2_map_lifecycle_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_map_view_tests map_view tests/map_view_tests.cpp)
target_link_libraries(ra2_map_view_tests PRIVATE ra2_core)
# Explicitly inspect session ownership and scoped Scenario services.
ra2_core_private_includes(ra2_map_view_tests)
ra2_add_native_test(ra2_theater_loading_tests theater_loading tests/theater_loading_tests.cpp)
target_link_libraries(ra2_theater_loading_tests PRIVATE ra2_core)
# Catalog verification binds only the explicit internal file-existence seam.
ra2_core_private_includes(ra2_theater_loading_tests)
ra2_add_native_test(ra2_map_loading_tests map_loading tests/map_loading_tests.cpp)
target_link_libraries(ra2_map_loading_tests PRIVATE ra2_core)
# Inspect real Cell/Scenario ownership and geometry after the public loader.
ra2_core_private_includes(ra2_map_loading_tests)
target_compile_definitions(ra2_map_resource_tests PRIVATE
    RA2_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")

ra2_add_native_test(ra2_map_compression_tests map_compression tests/map_compression_tests.cpp)
target_link_libraries(ra2_map_compression_tests PRIVATE ra2_core)
target_compile_definitions(ra2_map_compression_tests PRIVATE
    RA2_COMPRESSION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_compression_reference.txt")

# Public-header normalization and relocated-body regression tests.
ra2_add_native_test(ra2_yrpp_calling_convention_tests yrpp_calling_conventions
    tests/yrpp_calling_convention_tests.cpp)
target_link_libraries(ra2_yrpp_calling_convention_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_yrpp_header_cleanup_tests yrpp_header_cleanup
    tests/yrpp_header_cleanup_tests.cpp)
target_link_libraries(ra2_yrpp_header_cleanup_tests PRIVATE ra2_core)
find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME yrpp_header_style COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/check-yrpp-headers.py")
endif()

# Real local type lifecycles and renderer-neutral drawing; neither links the
# optional software component, so a missing backend cannot be hidden by it.
ra2_add_native_test(ra2_typeclass_native_tests typeclass_native tests/typeclass_native_tests.cpp)
target_link_libraries(ra2_typeclass_native_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_type_drawing_tests type_drawing tests/type_drawing_tests.cpp)
target_link_libraries(ra2_type_drawing_tests PRIVATE ra2_core)

if(Python3_Interpreter_FOUND)
    add_test(NAME typeclass_boundary_audit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/audit-typeclass-boundaries.py" --check)
    add_test(NAME map_boundary_audit COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/audit-map-boundaries.py" --check)
endif()

ra2_add_native_test(ra2_typeclass_lifecycle_tests typeclass_lifecycle tests/typeclass_lifecycle_tests.cpp)
target_link_libraries(ra2_typeclass_lifecycle_tests PRIVATE ra2_core)

add_executable(ra2_typeclass_ini_tests tests/typeclass_ini_tests.cpp)
target_link_libraries(ra2_typeclass_ini_tests PRIVATE ra2_core)
ra2_configure_cpp_target(ra2_typeclass_ini_tests)
ra2_use_googletest(ra2_typeclass_ini_tests)
add_test(NAME typeclass_ini COMMAND ra2_typeclass_ini_tests)

add_executable(ra2_typeclass_stream_tests tests/typeclass_stream_tests.cpp)
target_link_libraries(ra2_typeclass_stream_tests PRIVATE ra2_core)
ra2_configure_cpp_target(ra2_typeclass_stream_tests)
ra2_use_googletest(ra2_typeclass_stream_tests)
add_test(NAME typeclass_stream COMMAND ra2_typeclass_stream_tests)

ra2_add_native_test(ra2_radar_stream_tests radar_stream tests/radar_stream_tests.cpp)
target_link_libraries(ra2_radar_stream_tests PRIVATE ra2_core)

# Host-side metadata preparation is tested without Godot or software rendering.
ra2_add_native_test(ra2_type_drawing_packet_tests type_drawing_packets
    tests/type_drawing_packet_tests.cpp bridge/src/type_drawing_packets.cpp)
target_include_directories(ra2_type_drawing_packet_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src")
target_link_libraries(ra2_type_drawing_packet_tests PRIVATE ra2_core)
ra2_add_native_test(ra2_smudge_drawing_tests smudge_drawing
    tests/smudge_drawing_tests.cpp bridge/src/type_drawing_packets.cpp)
target_include_directories(ra2_smudge_drawing_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src")
target_link_libraries(ra2_smudge_drawing_tests PRIVATE ra2_core)
# Explicit access to the borrowed Cell drawing binding.
ra2_core_private_includes(ra2_smudge_drawing_tests)
add_executable(ra2_shape_gpu_fixture tests/shape_gpu_fixture.cpp bridge/src/type_drawing_packets.cpp)
# Explicit test-only access to the private host lighting geometry/bin encoder.
add_executable(ra2_lighting_gpu_fixture tests/lighting_gpu_fixture.cpp bridge/src/lighting_batch.cpp)
target_include_directories(ra2_lighting_gpu_fixture PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src")
target_link_libraries(ra2_lighting_gpu_fixture PRIVATE ra2_core)
target_include_directories(ra2_shape_gpu_fixture PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src")
target_link_libraries(ra2_shape_gpu_fixture PRIVATE ra2_core)
# Opt-in real-asset differential. Explicit test-only access to the existing
# voxel rasterizer and bridge packet encoder; never a normal host dependency.
add_executable(ra2_voxel_raster_fixture tests/voxel_raster_fixture.cpp bridge/src/type_drawing_packets.cpp)
ra2_configure_cpp_target(ra2_voxel_raster_fixture)
ra2_core_private_includes(ra2_voxel_raster_fixture)
target_include_directories(ra2_voxel_raster_fixture PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src")
target_link_libraries(ra2_voxel_raster_fixture PRIVATE ra2_core)
add_executable(ra2_voxel_bitmap_fixture tests/voxel_bitmap_fixture.cpp bridge/src/type_drawing_packets.cpp)
ra2_configure_cpp_target(ra2_voxel_bitmap_fixture)
target_include_directories(ra2_voxel_bitmap_fixture PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src")
target_link_libraries(ra2_voxel_bitmap_fixture PRIVATE ra2_core)
if(TARGET ra2_software_render)
    target_link_libraries(ra2_voxel_bitmap_fixture PRIVATE ra2_software_render)
    target_compile_definitions(ra2_voxel_bitmap_fixture PRIVATE RA2_VOXEL_BITMAP_SOFTWARE=1)
endif()
ra2_add_native_test(ra2_building_voxel_transform_tests building_voxel_transforms tests/building_voxel_transform_tests.cpp)
ra2_core_private_includes(ra2_building_voxel_transform_tests)
target_link_libraries(ra2_building_voxel_transform_tests PRIVATE ra2_core)
target_compile_definitions(ra2_building_voxel_transform_tests PRIVATE
    RA2_FLY_REFERENCE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fly_reference.txt"
    RA2_VOXEL_TRANSFORM_REFERENCE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_voxel_transform_reference.txt"
    RA2_UNIT_VOXEL_TRANSFORM_REFERENCE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_voxel_transform_reference.txt")
if(TARGET ra2_software_render)
    add_executable(ra2_map_drawing_reference tests/map_drawing_reference.cpp)
    # Explicit test-only access to the normal MapView/Scenario scope for side fixtures.
    target_include_directories(ra2_map_drawing_reference PRIVATE core/src)
    target_compile_definitions(ra2_map_drawing_reference PRIVATE
        RA2_UI_Z_GRADIENTS="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_ui_z_gradients.txt")
    ra2_configure_cpp_target(ra2_map_drawing_reference)
    target_link_libraries(ra2_map_drawing_reference PRIVATE ra2_software_render)
endif()

ra2_add_native_test(ra2_map_world_tests map_world tests/map_world_tests.cpp)
target_link_libraries(ra2_map_world_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_map_world_tests)
target_compile_definitions(ra2_map_world_tests PRIVATE
    RA2_BUILDING_COORDINATES_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_coordinates_reference.txt")

# Explicit test-only access: native map ownership, scoped INI/resource services,
# and renderer-neutral requests; never a Godot-side duplicate object state.
ra2_add_native_test(ra2_map_fidelity_tests map_fidelity_all tests/map_fidelity_tests.cpp)
target_link_libraries(ra2_map_fidelity_tests PRIVATE ra2_core)
ra2_core_private_includes(ra2_map_fidelity_tests)
target_compile_definitions(ra2_map_fidelity_tests PRIVATE
    RA2_BRIDGE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bridge_drawing_reference.txt"
    RA2_BUILDING_DRAW_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_drawing_reference.txt"
    RA2_TURRET_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_turret_reference.txt")
set_tests_properties(map_fidelity_all PROPERTIES LABELS "map-fidelity")

# Diagnostic-only access to actual native type/instance/asset state.
# Ordinary callers and the Godot bridge remain on the public API.
add_executable(ra2_map_fidelity_probe tests/map_fidelity_probe.cpp)
ra2_configure_cpp_target(ra2_map_fidelity_probe)
ra2_core_private_includes(ra2_map_fidelity_probe)
target_link_libraries(ra2_map_fidelity_probe PRIVATE ra2_core)

# V2: original-class world geometry, VXL/HVA and both host submission contracts.
add_executable(ra2_building_visual_tests tests/building_visual_tests.cpp)
ra2_configure_cpp_target(ra2_building_visual_tests)
# This suite has an explicit process partition even in discovery mode.
ra2_use_googletest(ra2_building_visual_tests NO_DISCOVERY)
add_test(NAME building_visual_all COMMAND ra2_building_visual_tests
    --gtest_filter=-BuildingVisual.TrigInitializationNearestPreservesRounding:BuildingVisual.TrigInitializationChoppedPreservesRounding)
# Each mode must initialize the function-local trig table in a fresh process.
foreach(case_id TrigInitializationNearestPreservesRounding TrigInitializationChoppedPreservesRounding)
    add_test(NAME visual_${case_id} COMMAND ra2_building_visual_tests --case ${case_id})
    set_tests_properties(visual_${case_id} PROPERTIES LABELS "map-fidelity;building-visual")
endforeach()
ra2_core_private_includes(ra2_building_visual_tests)
target_link_libraries(ra2_building_visual_tests PRIVATE ra2_core)
if(TARGET ra2_software_render)
 ra2_add_native_test(ra2_ore_visual_tests ore_visual tests/ore_visual_tests.cpp)
 ra2_core_private_includes(ra2_ore_visual_tests)
 target_link_libraries(ra2_ore_visual_tests PRIVATE ra2_core ra2_software_render)
 target_compile_definitions(ra2_ore_visual_tests PRIVATE RA2_ORE_Z_GRADIENTS="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_ui_z_gradients.txt")
 target_link_libraries(ra2_building_visual_tests PRIVATE ra2_software_render)
 target_compile_definitions(ra2_building_visual_tests PRIVATE RA2_VISUAL_SOFTWARE=1)
endif()
target_include_directories(ra2_building_visual_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src")
target_sources(ra2_building_visual_tests PRIVATE bridge/src/type_drawing_packets.cpp)
target_compile_definitions(ra2_building_visual_tests PRIVATE RA2_SELECTION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_selection_reference.txt")
target_compile_definitions(ra2_building_visual_tests PRIVATE RA2_HEALTH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/building_health_reference.txt")
set_tests_properties(building_visual_all PROPERTIES LABELS "map-fidelity;building-visual")
if(TARGET ra2_software_render)
    set_property(TEST building_visual_all APPEND PROPERTY LABELS software)
endif()

ra2_add_native_test(ra2_time_tests original_time tests/time_tests.cpp)
ra2_core_private_includes(ra2_time_tests)
target_link_libraries(ra2_time_tests PRIVATE ra2_core)
target_compile_definitions(ra2_time_tests PRIVATE RA2_TIME_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/time_reference.txt")

ra2_add_native_test(ra2_tooltip_tests tooltip tests/tooltip_tests.cpp)
target_link_libraries(ra2_tooltip_tests PRIVATE ra2_core)
# Explicit access to the native cursor/timer adapter and typed UI drawing scope.
ra2_core_private_includes(ra2_tooltip_tests)
target_compile_definitions(ra2_tooltip_tests PRIVATE
    RA2_TOOLTIP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tooltip_reference.txt")

# YRpp declaration/virtual-slot checks need no injector or game process.
if(WIN32 AND MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    include("${CMAKE_CURRENT_LIST_DIR}/MsvcAbiTests.cmake")
endif()
