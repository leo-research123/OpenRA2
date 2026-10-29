find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME debug_command_discovery COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/build/debug_command_discovery_tests.py"
        --cmake "${CMAKE_COMMAND}" --compiler "${CMAKE_CXX_COMPILER}" --generator "${CMAKE_GENERATOR}")
    set_tests_properties(debug_command_discovery PROPERTIES TIMEOUT 120 LABELS "product;build")
    add_test(NAME godot_build_profile COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/build/check_godot_build_profile.py" --cmake "${CMAKE_COMMAND}")
    set_tests_properties(godot_build_profile PROPERTIES LABELS "product;build")
    add_test(NAME render_layer_guard COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_render_layers.py" --root "${CMAKE_CURRENT_SOURCE_DIR}/.." --enforce)
    set_tests_properties(render_layer_guard PROPERTIES LABELS "product;build")
endif()
find_program(RA2_CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(RA2_CLANG_FORMAT_EXECUTABLE)
    file(GLOB ra2_shader_stage_sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/*.comp"
        "${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/*.frag")
    add_test(NAME render_shader_format COMMAND "${RA2_CLANG_FORMAT_EXECUTABLE}"
        --dry-run -Werror ${ra2_shader_stage_sources})
    set_tests_properties(render_shader_format PROPERTIES LABELS "product;build")
endif()
