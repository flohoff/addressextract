cmake_minimum_required (VERSION 3.18.4)
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

add_executable(addressextract addressextract.cpp AbstractArea.cpp Address.cpp)
include_directories(${Boost_INCLUDE_DIRS} ${JSON_INCLUDE_DIR} ${OSMIUM_INCLUDE_DIRS})

target_link_libraries(addressextract ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})
