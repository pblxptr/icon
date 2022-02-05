cmake_minimum_required(VERSION 3.18)
project(icon CXX)
set(CMAKE_CXX_STANDARD 20)

set(ICON_VERSION 0.1)

option(ENABLE_TESTING "Enable building and executing tests" OFF)

set(CMAKE_CXX_FLAGS "-Wno-deprecated")

include(cmake/StaticAnalyzers.cmake)

# -----------------------------------------------------------------------
# Workaround for clang, Boost asio has relased coroutines without fully supported clang
# -----------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)

add_subdirectory(icon)

if (ENABLE_TESTING)
  include_directories(${CMAKE_BINARY_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif()
