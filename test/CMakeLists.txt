add_executable(unittests
	test_address.cpp
	test_areas.cpp
	test_areaindex.cpp)
target_link_libraries(unittests addresscore Catch2::Catch2WithMain ${OSMIUM_LIBRARIES})
target_include_directories(unittests PRIVATE ${CMAKE_SOURCE_DIR})

include(Catch)
catch_discover_tests(unittests)
