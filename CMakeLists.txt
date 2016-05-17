cmake_minimum_required(VERSION 3.2)
project(vector_tile)

option(VECTOR_TILE_BUILD_TESTS "Build the geometry test program" ON)

# configure optimization
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPTIMIZATION_FLAGS "-O0") # optional: -fsanitize=signed-integer-overflow,undefined
    set(IS_DEBUG true)
    message("-- Configuring debug build")
else()
    set(OPTIMIZATION_FLAGS "-O3")
    set(IS_DEBUG false)
    message("-- Configuring release build")
endif()

# check if clang or gcc
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang$")
    set(CPP14_FLAG "-std=c++1y") # clang 3.4 compat
    set(CXX_COMPILER_IS_GNU_OR_CLANG true)
elseif (CMAKE_COMPILER_IS_GNUCXX)
    set(CPP14_FLAG "-std=c++14")
    set(CXX_COMPILER_IS_GNU_OR_CLANG true)
endif()
if ("${CMAKE_CC_COMPILER_ID}" STREQUAL "Clang")
    set(CC_COMPILER_IS_GNU_OR_CLANG true)
elseif (CMAKE_COMPILER_IS_GNUCC)
    set(CC_COMPILER_IS_GNU_OR_CLANG true)
endif()

# setup compiler flags
if(CC_COMPILER_IS_GNU_OR_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTIMIZATION_FLAGS}")
endif()
if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    endif()
elseif(CXX_COMPILER_IS_GNU_OR_CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPP14_FLAG} -pipe -Werror")
    # optional: -pedantic -Wconversion -Wall -Wextra
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPTIMIZATION_FLAGS}")
    # optional: -march=native (builds with the optimizations available on the build machine (only for local use!))
else()
    message(WARNING "Configuring earcut for unsupported compiler ${CMAKE_CXX_COMPILER_ID}.")
endif()

# Vector Tile
list(APPEND VECTOR_TILE_INCLUDE_DIRS "include")

include_directories(${VECTOR_TILE_INCLUDE_DIRS})

file(GLOB FIXTURE_SOURCE_FILES test/fixtures/*.cpp test/fixtures/*.hpp)
source_group(fixtures FILES ${FIXTURE_SOURCE_FILES})

set(COMMON_SOURCE_FILES ${FIXTURE_SOURCE_FILES})

set(TESTS_SOURCE_FILES test/tap.cpp test/tap.hpp test/test.cpp)

if (VECTOR_TILE_BUILD_TESTS)
    add_executable(tests ${TESTS_SOURCE_FILES})
endif()
