cmake_minimum_required (VERSION 3.19)
project (addressextract)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Osmium REQUIRED COMPONENTS io gdal)

if(NOT OSMIUM_FOUND)
	message(WARNING "Libosmium not found!\n")
endif()

find_package(Boost CONFIG 1.70.0 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_path(BOOST_GEOMETRY_INCLUDE_DIR
	NAMES boost/geometry/index/rtree.hpp
	HINTS ${Boost_INCLUDE_DIRS})
if(NOT BOOST_GEOMETRY_INCLUDE_DIR)
	message(FATAL_ERROR "Boost.Geometry headers not found (needs boost/geometry/index/rtree.hpp)")
endif()

find_path(JSON_INCLUDE_DIR
	NAMES "nlohmann/json.hpp"
	PATHS
	/usr/include/
	/usr/include/
	${CMAKE_SOURCE_DIR}/json/include
	${CMAKE_SYSROOT}/usr/local/include
	)
message("nlohmann directory ${JSON_INCLUDE_DIR}")

include_directories(${Boost_INCLUDE_DIRS} ${JSON_INCLUDE_DIR} ${OSMIUM_INCLUDE_DIRS})

# Code shared between the program and the unit tests
add_library(addresscore STATIC AbstractArea.cpp Address.cpp)
target_link_libraries(addresscore ${OSMIUM_LIBRARIES})

add_executable(addressextract addressextract.cpp)
target_link_libraries(addressextract addresscore ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})

# Unit tests: ctest, JUnit output: ctest --output-junit junit.xml
option(ENABLE_COVERAGE "Build with gcov instrumentation" OFF)
option(BUILD_TESTING "Build the unit tests" ON)

if(ENABLE_COVERAGE)
	set(COVERAGE_FLAGS --coverage -O0 -g)
	target_compile_options(addresscore PUBLIC ${COVERAGE_FLAGS})
	target_link_options(addresscore PUBLIC --coverage)
	target_compile_options(addressextract PRIVATE ${COVERAGE_FLAGS})
	target_link_options(addressextract PRIVATE --coverage)
endif()

if(BUILD_TESTING)
	find_package(Catch2 3 QUIET)
	if(Catch2_FOUND)
		enable_testing()
		add_subdirectory(test)

		if(ENABLE_COVERAGE)
			find_program(GCOVR gcovr)
			if(GCOVR)
				# make coverage: runs the tests, writes coverage.xml (Cobertura) and a summary
				add_custom_target(coverage
					COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --output-junit junit.xml
					COMMAND ${GCOVR} --root ${CMAKE_SOURCE_DIR}
						--filter ${CMAKE_SOURCE_DIR}/.*
						--exclude ${CMAKE_SOURCE_DIR}/test/.*
						--object-directory ${CMAKE_BINARY_DIR}
						--print-summary --xml coverage.xml
						${CMAKE_BINARY_DIR}
					WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
					VERBATIM
					COMMENT "Running tests and collecting coverage")
			else()
				message(WARNING "gcovr not found, no coverage target")
			endif()
		endif()
	else()
		message(STATUS "Catch2 v3 not found - unit tests disabled")
	endif()
endif()
