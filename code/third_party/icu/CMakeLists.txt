# Build adapter for unmodified upstream ICU sources. The source archive contains
# the complete conversion data, so no host tools, configure or make are needed.
include(FetchContent)
FetchContent_Declare(ra2_icu
    URL https://github.com/unicode-org/icu/releases/download/release-78.3/icu4c-78.3-sources.tgz
    URL_HASH SHA256=3a2e7a47604ba702f345878308e6fefeca612ee895cf4a5f222e7955fabfe0c0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR ra2-no-upstream-cmake)
FetchContent_MakeAvailable(ra2_icu)
set(icu_source "${ra2_icu_SOURCE_DIR}/source")
file(STRINGS "${icu_source}/common/unicode/uvernum.h" icu_version
    REGEX "^#define U_ICU_VERSION +\"78\\.3\"$")
if(NOT icu_version)
    message(FATAL_ERROR "The bundled ICU adapter requires the pinned ICU 78.3 sources.")
endif()

enable_language(C)
if(NOT CMAKE_C_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
    message(FATAL_ERROR "The pinned ICU conversion data supports little-endian desktop targets.")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
if(NOT WIN32)
    find_package(Threads REQUIRED)
endif()
set(icu_data "${icu_source}/data/in/icudt78l.dat")
set(icu_data_c "${CMAKE_CURRENT_BINARY_DIR}/icu_data.c")
add_custom_command(OUTPUT "${icu_data_c}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_data.py"
        "${icu_data}" "${icu_data_c}"
    DEPENDS "${icu_data}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_data.py"
    COMMENT "Embedding pinned ICU conversion data"
    VERBATIM)
add_library(ra2_icu_data STATIC "${icu_data_c}")
add_library(ICU::data ALIAS ra2_icu_data)

# Read upstream's authoritative lists rather than maintaining another copy.
foreach(component IN ITEMS common i18n)
    file(STRINGS "${icu_source}/${component}/sources.txt" sources)
    list(TRANSFORM sources PREPEND "${icu_source}/${component}/")
    if(component STREQUAL "common")
        set(target ra2_icu_uc)
        set(implementation U_COMMON_IMPLEMENTATION)
    else()
        set(target ra2_icu_i18n)
        set(implementation U_I18N_IMPLEMENTATION)
    endif()
    add_library(${target} STATIC ${sources})
    target_compile_definitions(${target} PRIVATE ${implementation} U_ATTRIBUTE_DEPRECATED=)
    target_include_directories(${target} PRIVATE "${icu_source}/${component}")
    target_include_directories(${target} SYSTEM PUBLIC "${icu_source}/common")
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /EHsc)
    endif()
endforeach()
add_library(ICU::uc ALIAS ra2_icu_uc)
add_library(ICU::i18n ALIAS ra2_icu_i18n)
target_include_directories(ra2_icu_i18n SYSTEM PUBLIC "${icu_source}/i18n")
target_include_directories(ra2_icu_data SYSTEM PRIVATE "${icu_source}/common")
target_link_libraries(ra2_icu_uc PUBLIC ra2_icu_data)
if(NOT WIN32)
    target_link_libraries(ra2_icu_uc PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()
target_link_libraries(ra2_icu_i18n PUBLIC ra2_icu_uc)
if(WIN32)
    target_link_libraries(ra2_icu_uc PRIVATE advapi32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(ra2_icu_uc PRIVATE m)
endif()
foreach(target IN ITEMS ra2_icu_data ra2_icu_uc ra2_icu_i18n)
    target_compile_definitions(${target} PUBLIC U_STATIC_IMPLEMENTATION)
    set_target_properties(${target} PROPERTIES
        POSITION_INDEPENDENT_CODE ON C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON
        MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
endforeach()
set_target_properties(ra2_icu_data PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
message(STATUS "ICU: bundled 78.3 sources (${ra2_icu_SOURCE_DIR})")
