cmake_minimum_required(VERSION 3.18)
project(icon CXX)
set(CMAKE_CXX_STANDARD 20)

set(Upstream_VERSION 0.1)

option(ENABLE_TESTING "Enable building and executing tests" OFF)

set(CMAKE_CXX_FLAGS "-Wno-deprecated")

include(cmake/StaticAnalyzers.cmake)

add_subdirectory(icon)

if (ENABLE_TESTING)
  include_directories(${CMAKE_BINARY_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif()
