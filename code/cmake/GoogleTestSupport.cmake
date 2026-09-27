include_guard(GLOBAL)

# A system package or an existing parent-provided target can be used offline.
# Otherwise download the pinned upstream release, and only when testing is on.
if(NOT TARGET GTest::gtest)
    find_package(GTest 1.14 QUIET CONFIG)
endif()
if(NOT TARGET GTest::gtest)
    include(FetchContent)
    set(BUILD_GMOCK OFF CACHE BOOL "Build GoogleMock" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "Install vendored GoogleTest" FORCE)
    # Honor the application's /MD vs /MT policy on the Microsoft ABI builds.
    if(CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
        set(gtest_force_shared_crt ON CACHE BOOL "Match the RA2 CRT" FORCE)
    else()
        set(gtest_force_shared_crt OFF CACHE BOOL "Match the RA2 CRT" FORCE)
    endif()
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/v1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(googletest)
endif()

get_filename_component(_ra2_gtest_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_library(ra2_test_support STATIC "${_ra2_gtest_root}/tests/support/test_support.cpp")
ra2_configure_cpp_target(ra2_test_support)
target_include_directories(ra2_test_support PUBLIC "${_ra2_gtest_root}/tests")
target_link_libraries(ra2_test_support PUBLIC GTest::gtest)
add_library(ra2_test_main STATIC "${_ra2_gtest_root}/tests/support/test_main.cpp")
ra2_configure_cpp_target(ra2_test_main)
target_link_libraries(ra2_test_main PUBLIC ra2_test_support)

# Default registration retains the old CTest process boundaries, names, labels,
# timeouts and x86 command probes. Fine-grained discovery is opt-in because it
# creates additional CTest entries and can re-run stateful aggregate contracts.
option(RA2_GTEST_DISCOVER_TESTS "Also expose individual GoogleTest cases in CTest" OFF)
include(GoogleTest)
function(ra2_use_googletest target)
    cmake_parse_arguments(GTEST "NO_MAIN;NO_DISCOVERY" "" "EXTRA_ARGS" ${ARGN})
    if(GTEST_NO_MAIN)
        target_link_libraries(${target} PRIVATE ra2_test_support)
    else()
        target_link_libraries(${target} PRIVATE ra2_test_main)
    endif()
    if(RA2_GTEST_DISCOVER_TESTS AND NOT GTEST_NO_DISCOVERY)
        # Do not try to execute target Windows binaries on a cross-build host.
        if(NOT CMAKE_CROSSCOMPILING OR CMAKE_CROSSCOMPILING_EMULATOR)
            gtest_discover_tests(${target}
                TEST_PREFIX "${target}."
                EXTRA_ARGS ${GTEST_EXTRA_ARGS}
                DISCOVERY_MODE PRE_TEST)
        endif()
    endif()
endfunction()
