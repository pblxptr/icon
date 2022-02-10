cmake_minimum_required(VERSION 3.18)
project(icon CXX)
set(CMAKE_CXX_STANDARD 20)

set(ICON_VERSION 0.1)

option(ENABLE_TESTING "Enable building and executing tests" OFF)

add_compile_options("-Wno-deprecated")

include(cmake/StaticAnalyzers.cmake)

# -----------------------------------------------------------------------
# Workaround for clang, Boost asio has relased coroutines without fully supported clang
# -----------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)


set(Protobuf_DEBUG TRUE)

find_package(Protobuf REQUIRED)
set(PROTOBUF_GENERATE_CPP_APPEND_PATH FALSE)
set(Protobuf_IMPORT_DIRS ${CMAKE_SOURCE_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  ${CMAKE_SOURCE_DIR}/icon/metadata/metadata.proto
  ${CMAKE_SOURCE_DIR}/icon/protobuf/icon.proto
)



add_subdirectory(icon)

if (ENABLE_TESTING)
  include_directories(${CMAKE_BINARY_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif()
