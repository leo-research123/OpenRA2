# Build the pinned, unmodified shader compiler for the build host, not the target.
include(FetchContent)
include(ExternalProject)
set(ra2_glslang_commit 46ef757e048e760b46601e6e77ae0cb72c97bd2f)
FetchContent_Declare(ra2_glslang
    URL "https://github.com/KhronosGroup/glslang/archive/${ra2_glslang_commit}.tar.gz"
    URL_HASH SHA256=ef16e5924e07f49b6280158645051ed5930ee5c5628e573fd2fb49e210aaca71
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR ra2-no-upstream-cmake)
FetchContent_MakeAvailable(ra2_glslang)
file(STRINGS "${ra2_glslang_SOURCE_DIR}/CHANGES.md" ra2_glslang_version
    REGEX "^#+ *15\\.0\\.0( |$)" LIMIT_COUNT 1)
if(NOT ra2_glslang_version)
    message(FATAL_ERROR "The bundled shader compiler requires the pinned glslang 15.0.0 sources.")
endif()

set(ra2_glslang_host_binary "${CMAKE_CURRENT_BINARY_DIR}/host/bin/glslang")
if(CMAKE_HOST_WIN32)
    string(APPEND ra2_glslang_host_binary ".exe")
endif()
set(ra2_glslang_host_args)
# The parent may use a target toolchain, compiler, architecture or sysroot.
# Only the build program is shared; a separate native project discovers its host SDK.
if(CMAKE_MAKE_PROGRAM)
    list(APPEND ra2_glslang_host_args "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}")
endif()
set(RA2_GLSLANG_HOST_C_COMPILER "" CACHE FILEPATH "Optional native C compiler for bundled glslang")
set(RA2_GLSLANG_HOST_CXX_COMPILER "" CACHE FILEPATH "Optional native C++ compiler for bundled glslang")
foreach(language C CXX)
    if(RA2_GLSLANG_HOST_${language}_COMPILER)
        list(APPEND ra2_glslang_host_args
            "-DCMAKE_${language}_COMPILER:FILEPATH=${RA2_GLSLANG_HOST_${language}_COMPILER}")
    endif()
endforeach()
ExternalProject_Add(ra2_glslang_host_build
    SOURCE_DIR "${ra2_glslang_SOURCE_DIR}"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/host"
    STAMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/stamp-${ra2_glslang_commit}"
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env
        --unset=CMAKE_TOOLCHAIN_FILE --unset=CC --unset=CXX
        --unset=CFLAGS --unset=CXXFLAGS --unset=CPPFLAGS --unset=LDFLAGS --unset=SDKROOT
        "${CMAKE_COMMAND}" -S <SOURCE_DIR> -B <BINARY_DIR> -G "${CMAKE_GENERATOR}"
        ${ra2_glslang_host_args}
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=<BINARY_DIR>/bin
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE:PATH=<BINARY_DIR>/bin
        -DBUILD_SHARED_LIBS:BOOL=OFF -DBUILD_EXTERNAL:BOOL=OFF
        -DGLSLANG_TESTS:BOOL=OFF -DGLSLANG_ENABLE_INSTALL:BOOL=OFF
        -DENABLE_GLSLANG_BINARIES:BOOL=ON -DENABLE_SPIRV:BOOL=ON
        -DENABLE_OPT:BOOL=OFF -DENABLE_HLSL:BOOL=OFF -DENABLE_SPVREMAPPER:BOOL=OFF
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --config Release
        --target glslang-standalone --parallel 2
    BUILD_BYPRODUCTS "${ra2_glslang_host_binary}"
    INSTALL_COMMAND "")
add_executable(ra2::glslang_host IMPORTED GLOBAL)
set_target_properties(ra2::glslang_host PROPERTIES IMPORTED_LOCATION "${ra2_glslang_host_binary}")
add_dependencies(ra2::glslang_host ra2_glslang_host_build)
set(ra2_shader_compiler "$<TARGET_FILE:ra2::glslang_host>" PARENT_SCOPE)
set(ra2_shader_compiler_dependencies ra2_glslang_host_build "${ra2_glslang_host_binary}" PARENT_SCOPE)
message(STATUS "Shader compiler: bundled host glslang 15.0.0 (${ra2_glslang_SOURCE_DIR})")
