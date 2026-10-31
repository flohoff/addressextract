add_executable(unittests
	test_address.cpp
	test_areas.cpp
	test_areaindex.cpp)
target_link_libraries(unittests addresscore Catch2::Catch2WithMain ${OSMIUM_LIBRARIES})
target_include_directories(unittests PRIVATE ${CMAKE_SOURCE_DIR})

include(Catch)
catch_discover_tests(unittests)

# End to end runs on synthetic.osm vs golden output
set(SYNTH ${CMAKE_CURRENT_SOURCE_DIR}/data/synthetic.osm)
function(golden_test name)
	# EXPECT: use another golden file
	set(expect ${name})
	if(EXPECT)
		set(expect ${EXPECT})
	endif()
	add_test(NAME golden_${name}
		COMMAND ${CMAKE_COMMAND}
			-DEXE=$<TARGET_FILE:addressextract>
			-DINPUT=${SYNTH}
			-DOUT=${CMAKE_CURRENT_BINARY_DIR}/out_${name}.json
			-DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/data/expected_${expect}.json
			"-DFLAGS=${ARGN}"
			-DOSMIUM=${OSMIUM_PROGRAM}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/run_golden.cmake)
endfunction()

golden_test(plain)
golden_test(errors -e)
golden_test(missing -m)
# --nocache must match the default run
set(EXPECT plain)
golden_test(plain_nocache --nocache)
set(EXPECT errors)
golden_test(errors_nocache -e --nocache)
set(EXPECT missing)
golden_test(missing_nocache -m --nocache)
unset(EXPECT)

# PBF input (header timestamp), needs the osmium tool
find_program(OSMIUM_TOOL osmium)
if(OSMIUM_TOOL)
	set(OSMIUM_PROGRAM ${OSMIUM_TOOL})
	golden_test(errors_pbf -e)
	unset(OSMIUM_PROGRAM)
else()
	message(STATUS "osmium not found - pbf test disabled")
endif()

# Command line handling
add_test(NAME cli_help COMMAND addressextract --help)
set_tests_properties(cli_help PROPERTIES WILL_FAIL TRUE)   # exits 1
add_test(NAME cli_missing_args COMMAND addressextract)
set_tests_properties(cli_missing_args PROPERTIES WILL_FAIL TRUE)
