cmake_minimum_required(VERSION 3.22)
project(zmq_boost CXX)

set(CMAKE_CXX_STANDARD 20)

include(cmake/StaticAnalyzers.cmake)

find_package(Catch2 3 REQUIRED)
find_package(Boost REQUIRED coroutine)
find_package(Protobuf REQUIRED)

include_directories(build/src)
include_directories(build/tests)

add_subdirectory(src)
add_subdirectory(tests)