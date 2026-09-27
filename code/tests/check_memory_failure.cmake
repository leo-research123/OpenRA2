execute_process(COMMAND "${PROBE}" ${PROBE_ARGUMENT} RESULT_VARIABLE result)
if(NOT "${result}" STREQUAL "${EXPECTED}")
    message(FATAL_ERROR "Checked allocation exit was ${result}; expected ${EXPECTED}")
endif()
