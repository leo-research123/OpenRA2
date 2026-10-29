ra2_add_product_contract(ra2_runtime_diagnostics_tests runtime_diagnostics tests/unit/diagnostics/runtime_diagnostics_tests.cpp)
target_link_libraries(ra2_runtime_diagnostics_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_runtime_diagnostics_tests)

ra2_add_product_contract(ra2_draw_resource_pool_tests draw_resource_pool tests/unit/render/draw_resource_pool_tests.cpp)
target_link_libraries(ra2_draw_resource_pool_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_draw_resource_pool_tests)
target_include_directories(ra2_draw_resource_pool_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/render/src")
set_property(TEST draw_resource_pool APPEND PROPERTY LABELS render-cpu)

ra2_add_product_contract(ra2_scene_row_table_tests scene_row_table tests/unit/render/scene_row_table_tests.cpp)
target_link_libraries(ra2_scene_row_table_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_scene_row_table_tests)
target_include_directories(ra2_scene_row_table_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/render/src")
set_property(TEST scene_row_table APPEND PROPERTY LABELS render-cpu)

ra2_add_product_contract(ra2_pixel_run_tests pixel_run tests/unit/render/pixel_run_tests.cpp)
target_link_libraries(ra2_pixel_run_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_pixel_run_tests)
target_include_directories(ra2_pixel_run_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/render/src")
set_property(TEST pixel_run APPEND PROPERTY LABELS render-cpu)

ra2_add_product_contract(ra2_voxel_resource_tests voxel_resource tests/unit/render/voxel_resource_tests.cpp)
target_link_libraries(ra2_voxel_resource_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_voxel_resource_tests)
target_include_directories(ra2_voxel_resource_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/render/src")
set_property(TEST voxel_resource APPEND PROPERTY LABELS render-cpu)

ra2_add_product_contract(ra2_audio_mixer_tests audio_mixer tests/unit/audio/mixer_tests.cpp)
target_link_libraries(ra2_audio_mixer_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_audio_mixer_tests)

ra2_add_product_contract(ra2_file_chain_tests file_chain tests/unit/filesystem/file_chain_tests.cpp)
target_link_libraries(ra2_file_chain_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_file_chain_tests)

ra2_add_product_contract(ra2_raw_file_storage_tests raw_file_storage tests/unit/filesystem/raw_file_storage_tests.cpp)
target_link_libraries(ra2_raw_file_storage_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_raw_file_storage_tests)

ra2_add_product_contract(ra2_point_grid_tests point_grid tests/unit/render/point_grid_tests.cpp)
# Private header-only spatial index, checked against exhaustive point membership.
ra2_core_private_includes(ra2_point_grid_tests)
set_property(TEST point_grid APPEND PROPERTY LABELS render-cpu)

ra2_add_product_contract(ra2_frame_diagnostics_tests frame_diagnostics tests/unit/diagnostics/frame_diagnostics_tests.cpp)
target_link_libraries(ra2_frame_diagnostics_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_frame_diagnostics_tests)
target_include_directories(ra2_frame_diagnostics_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/bridge/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/src/presentation")
set_property(TEST frame_diagnostics APPEND PROPERTY LABELS render-cpu)

ra2_add_product_contract(ra2_projectile_log_tests projectile_log tests/unit/diagnostics/projectile_log_tests.cpp)
target_link_libraries(ra2_projectile_log_tests PRIVATE ra2_core)
# Explicit project implementation contracts; no reference corpus or game installation.
ra2_core_private_includes(ra2_projectile_log_tests)

ra2_add_product_contract(ra2_audio_session_contract_tests audio_session_contract tests/unit/audio/session_contract_tests.cpp)
target_link_libraries(ra2_audio_session_contract_tests PRIVATE ra2_core)
