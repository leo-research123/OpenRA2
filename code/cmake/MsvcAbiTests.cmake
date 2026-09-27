include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/GoogleTestSupport.cmake")
get_filename_component(_ra2_abi_native "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_executable(ra2_msvc_abi_contract_tests "${_ra2_abi_native}/tests/x86/msvc_abi_contract_tests.cpp")
# FileClass now owns out-of-line construction and default virtual methods.
target_sources(ra2_msvc_abi_contract_tests PRIVATE
    "${_ra2_abi_native}/core/src/yrpp/FileClass.cpp"
    "${_ra2_abi_native}/core/src/yrpp/Memory.cpp")
ra2_configure_cpp_target(ra2_msvc_abi_contract_tests)
ra2_use_googletest(ra2_msvc_abi_contract_tests)
ra2_core_private_includes(ra2_msvc_abi_contract_tests)
target_compile_definitions(ra2_msvc_abi_contract_tests PRIVATE RA2_YRPP_GAME)
add_test(NAME msvc_x86_abi COMMAND ra2_msvc_abi_contract_tests)
add_library(ra2_msvc_abi_codegen OBJECT "${_ra2_abi_native}/tests/x86/msvc_abi_codegen.cpp")
ra2_configure_cpp_target(ra2_msvc_abi_codegen)
ra2_core_private_includes(ra2_msvc_abi_codegen)
target_include_directories(ra2_msvc_abi_codegen PRIVATE "${_ra2_abi_native}/tests")
target_compile_definitions(ra2_msvc_abi_codegen PRIVATE RA2_YRPP_GAME)
# The object target also checks all layout static_asserts with native cl.exe.
# The IR checker uses clang-cl and compile_commands.json (Ninja/Makefiles only).
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_target(ra2_check_msvc_abi ALL
        COMMAND "${Python3_EXECUTABLE}" "${_ra2_abi_native}/../scripts/check-msvc-abi.py"
            --compile-commands "${CMAKE_BINARY_DIR}/compile_commands.json"
            --output "${CMAKE_BINARY_DIR}/msvc-abi-report.json"
        DEPENDS ra2_msvc_abi_codegen
        COMMENT "Checking real project virtual slots and x86 thiscall in LLVM IR"
        VERBATIM)
endif()

add_executable(ra2_msvc_calling_convention_tests
    "${_ra2_abi_native}/tests/yrpp_calling_convention_tests.cpp")
ra2_configure_cpp_target(ra2_msvc_calling_convention_tests)
ra2_use_googletest(ra2_msvc_calling_convention_tests)
ra2_core_private_includes(ra2_msvc_calling_convention_tests)
target_compile_definitions(ra2_msvc_calling_convention_tests PRIVATE RA2_YRPP_GAME)
add_test(NAME msvc_x86_calling_conventions COMMAND ra2_msvc_calling_convention_tests)
