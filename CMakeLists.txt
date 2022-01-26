cmake_minimum_required(VERSION 3.22)
project(icon CXX)


option(ENABLE_TESTING "Enable building and executing tests" OFF)

set(CMAKE_CXX_STANDARD 20)

include(cmake/StaticAnalyzers.cmake)

find_package(Boost REQUIRED coroutine)
find_package(Protobuf REQUIRED)

include_directories(${CMAKE_BINARY_DIR}/src)

add_subdirectory(src)

if (ENABLE_TESTING)
  include_directories(${CMAKE_BINARY_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif()
