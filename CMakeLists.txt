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
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libosmium/cmake")
find_package(Osmium REQUIRED COMPONENTS io gdal)

if(NOT OSMIUM_FOUND)
	message(WARNING "Libosmium not found!\n")
endif()
find_package(Boost 1.55.0 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_package(PkgConfig)
pkg_check_modules(LSI spatialindex)
IF(NOT LSI_FOUND)
	find_path(LSI_INCLUDE_DIRS
			NAMES SpatialIndex.h
			PATHS
			/usr/include/spatialindex
			/usr/include/spatialindex
			${CMAKE_SYSROOT}/usr/local/include
			${CMAKE_SYSROOT}/usr/local/include/spatialindex
	)
	find_library(LSI_LIBRARIES spatialindex)
ENDIF(NOT LSI_FOUND)

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
include_directories(${Boost_INCLUDE_DIRS} ${LSI_INCLUDE_DIRS} ${JSON_INCLUDE_DIR} ${OSMIUM_INCLUDE_DIRS})

target_link_libraries(addressextract ${Boost_LIBRARIES} ${LSI_LIBRARIES} ${OSMIUM_LIBRARIES})
