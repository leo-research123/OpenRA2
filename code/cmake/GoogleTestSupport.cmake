include_guard(GLOBAL)

# A system package or an existing parent-provided target can be used offline.
# Otherwise download the pinned upstream release, and only when testing is on.
if(NOT TARGET GTest::gtest)
    find_package(GTest 1.14 QUIET CONFIG)
endif()
if(NOT TARGET GTest::gtest)
    include(FetchContent)
    # Static-library try-compiles cannot detect missing pthread symbols when
    # cross-compiling for Windows. Use GoogleTest's native Windows threading.
    if(WIN32)
        set(gtest_disable_pthreads ON CACHE BOOL "Use Windows threading in GoogleTest" FORCE)
    endif()
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
    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 23)
        # Upstream enables /WX; Clang 23 diagnoses its UTF-8 printer conversion.
        target_compile_options(gtest PRIVATE /clang:-Wno-error=character-conversion)
    endif()
endif()

# Individual discovery is optional; aggregate process boundaries remain stable.
option(RA2_GTEST_DISCOVER_TESTS "Also expose individual GoogleTest cases in CTest" OFF)
include(GoogleTest)
