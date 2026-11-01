# Params: EXE INPUT OUT EXPECTED [FLAGS] [OSMIUM: convert INPUT to pbf first]
if(OSMIUM)
	set(pbf ${OUT}.pbf)
	execute_process(
		COMMAND ${OSMIUM} cat --overwrite -f pbf
			--output-header=osmosis_replication_timestamp=2026-01-01T00:00:00Z
			-o ${pbf} ${INPUT}
		RESULT_VARIABLE rc ERROR_VARIABLE err)
	if(NOT rc EQUAL 0)
		message(FATAL_ERROR "osmium failed (${rc}): ${err}")
	endif()
	set(INPUT ${pbf})
endif()

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
string(JSON naddr LENGTH "${content}" features)
message(STATUS "${naddr} addresses")

execute_process(
	COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${EXPECTED}
	RESULT_VARIABLE diff)
if(NOT diff EQUAL 0)
	message(FATAL_ERROR "Output ${OUT} differs from ${EXPECTED}")
endif()
