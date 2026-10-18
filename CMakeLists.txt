cmake_minimum_required(VERSION 3.16)
project(cpppattern_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror -O3")

file(GLOB_RECURSE SOURCES "test/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})
