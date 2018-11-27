cmake_minimum_required (VERSION 2.6)
project (addressextract)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Osmium REQUIRED COMPONENTS < pbf io gdal >)

if(NOT OSMIUM_FOUND)
	message(WARNING "Libosmium not found!\n")
endif()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

find_package(Boost 1.55.0 REQUIRED COMPONENTS program_options )
include_directories(${Boost_INCLUDE_DIRS})

find_package(PkgConfig)
pkg_check_modules(LSI REQUIRED libspatialindex)

add_executable(addressextract addressextract.cpp AbstractArea.cpp Area.cpp)
include_directories("${OSMIUM_INCLUDE_DIRS}")
include_directories(${Boost_INCLUDE_DIRS} ${LSI_INCLUDE_DIRS})
target_link_libraries(addressextract ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${LSI_LIBRARIES})
