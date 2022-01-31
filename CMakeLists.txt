cmake_minimum_required(VERSION 3.18)
project(icon CXX)
set(CMAKE_CXX_STANDARD 20)

option(ENABLE_TESTING "Enable building and executing tests" OFF)

find_package(Boost 1.78 REQUIRED coroutine)
find_package(Protobuf REQUIRED)

set(CMAKE_CXX_FLAGS "-Wno-deprecated")

include(cmake/StaticAnalyzers.cmake)

include_directories(${CMAKE_BINARY_DIR})

add_subdirectory(icon)

if (ENABLE_TESTING)
  include_directories(${CMAKE_BINARY_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif()
