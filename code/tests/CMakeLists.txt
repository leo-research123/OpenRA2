include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GoogleTestSupport.cmake")

# Public cases explain their inputs and expectations in the adjacent provenance files.
function(ra2_add_product_contract target name)
    add_executable(${target} ${ARGN})
    ra2_configure_cpp_target(${target})
    target_link_libraries(${target} PRIVATE GTest::gtest_main)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_test(NAME ${name} COMMAND ${target})
    set_tests_properties(${name} PROPERTIES LABELS product)
    if(RA2_GTEST_DISCOVER_TESTS AND (NOT CMAKE_CROSSCOMPILING OR CMAKE_CROSSCOMPILING_EMULATOR))
        gtest_discover_tests(${target} TEST_PREFIX "${target}." DISCOVERY_MODE PRE_TEST)
    endif()
endfunction()

include("${CMAKE_CURRENT_SOURCE_DIR}/tests/contracts/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/tests/build/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/BuildChecks.cmake")
