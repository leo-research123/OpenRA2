include_guard(GLOBAL)

# Only implementation targets and tests may opt into the internal contracts.
# Ordinary consumers obtain core/include from ra2_core's public usage requirements.
function(ra2_core_private_includes target)
    get_filename_component(native "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    target_include_directories(${target} PRIVATE
        "${native}/core/include" "${native}/core/src" "${native}")
endfunction()
