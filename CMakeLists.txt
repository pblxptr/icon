cmake_minimum_required(VERSION 3.22)
project(zmq_boost CXX)

include(cmake/StaticAnalyzers.cmake)

find_package(Boost REQUIRED coroutine)
find_package(Protobuf REQUIRED)

include_directories(build/src)


add_subdirectory(src)