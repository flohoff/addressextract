add_executable(unittests
	test_address.cpp
	test_areas.cpp
	test_areaindex.cpp)
target_link_libraries(unittests addresscore Catch2::Catch2WithMain ${OSMIUM_LIBRARIES})
target_include_directories(unittests PRIVATE ${CMAKE_SOURCE_DIR})

include(Catch)
catch_discover_tests(unittests)

# End to end runs on the synthetic OSM XML file, compared to golden output.
# Regenerate goldens after intended output changes:
#   addressextract [flags] -i data/synthetic.osm -o data/expected_<name>.json
set(SYNTH ${CMAKE_CURRENT_SOURCE_DIR}/data/synthetic.osm)
function(golden_test name)
	add_test(NAME golden_${name}
		COMMAND ${CMAKE_COMMAND}
			-DEXE=$<TARGET_FILE:addressextract>
			-DINPUT=${SYNTH}
			-DOUT=${CMAKE_CURRENT_BINARY_DIR}/out_${name}.json
			-DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/data/expected_${name}.json
			"-DFLAGS=${ARGN}"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/run_golden.cmake)
endfunction()

golden_test(plain)
golden_test(errors -e)
golden_test(missing -m)
golden_test(errors_nocache -e --nocache)
golden_test(missing_nocache -m --nocache)

# Command line handling
add_test(NAME cli_help COMMAND addressextract --help)
set_tests_properties(cli_help PROPERTIES WILL_FAIL TRUE)   # exits 1
add_test(NAME cli_missing_args COMMAND addressextract)
set_tests_properties(cli_missing_args PROPERTIES WILL_FAIL TRUE)
