# Params: EXE INPUT OUT EXPECTED [FLAGS]
execute_process(
	COMMAND ${EXE} ${FLAGS} -i ${INPUT} -o ${OUT}
	RESULT_VARIABLE rc
	ERROR_VARIABLE err
	OUTPUT_QUIET)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "addressextract failed (${rc}): ${err}")
endif()

# Must be valid JSON
file(READ ${OUT} content)
string(JSON naddr LENGTH "${content}" addresses)
message(STATUS "${naddr} addresses")

execute_process(
	COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${EXPECTED}
	RESULT_VARIABLE diff)
if(NOT diff EQUAL 0)
	message(FATAL_ERROR "Output ${OUT} differs from ${EXPECTED}")
endif()
