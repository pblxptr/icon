cmake_minimum_required(VERSION 3.22)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  ../src/protobuf/metadata.proto
  ../src/protobuf/icon.proto
  dummy.proto
)

set(APP_SOURCES
  ../src/endpoint/basic_endpoint/basic_endpoint.cpp
)

set(TEST_SOURCES
  basic_endpoint_tests.cpp
)

include_directories(../src/)

add_executable(${CMAKE_PROJECT_NAME}_test
  ${APP_SOURCES}
  ${TEST_SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME}_test
  PRIVATE Catch2::Catch2WithMain
  pthread
  zmq
  zmqpp
  fmt
  ${Protobuf_LIBRARIES}
)