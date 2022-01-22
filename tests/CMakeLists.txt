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
  send_receive_tests.cpp
  client/basic_client_tests.cpp
  client/request_tests.cpp
  client/response_tests.cpp
  utils/async_mutex_tests.cpp
)

include_directories(../src/)

add_executable(${CMAKE_PROJECT_NAME}_test
  ${APP_SOURCES}
  ${TEST_SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

target_link_libraries(${CMAKE_PROJECT_NAME}_test
  PRIVATE Catch2::Catch2WithMain
  pthread
  zmq
  zmqpp
  fmt
  ${Protobuf_LIBRARIES}
)