cmake_minimum_required(VERSION 3.10)

project(wave_function_collapse C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Wall -Wextra -g -pedantic")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -DNDEBUG -pedantic")

include_directories(${CMAKE_SOURCE_DIR}/src)

file(GLOB SOURCES "src/*.c")

add_executable(wfc ${SOURCES})
