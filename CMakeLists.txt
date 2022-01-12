cmake_minimum_required(VERSION 3.22)
project(zmq_boost CXX)

include(cmake/StaticAnalyzers.cmake)

find_package(Boost REQUIRED coroutine)
find_package(Protobuf REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  protobuf/metadata.proto
  protobuf/icon.proto
)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # set(CMAKE_CXX_FLAGS "-fcoroutines")
else()
  #Workaround for clang, Boost asio has relased coroutines without full supported clang
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()

set(SOURCES
  main.cpp
  utils/context.cpp
  endpoint/basic_endpoint/basic_endpoint.cpp
)
file(GLOB HEADER_SOURCES${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)

include_directories(build)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${CMAKE_PROJECT_NAME}
  ${SOURCES}
  ${HEADER_SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
  pthread
  zmq
  zmqpp
  fmt
  ${Protobuf_LIBRARIES}
)


