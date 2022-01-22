cmake_minimum_required(VERSION 3.22)

#Workaround for clang, Boost asio has relased coroutines without fully supported clang

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()

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

add_executable(${CMAKE_PROJECT_NAME}_unittests
  ${APP_SOURCES}
  ${TEST_SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

target_link_libraries(${CMAKE_PROJECT_NAME}_unittests
  PRIVATE Catch2::Catch2WithMain
  pthread
  zmq
  zmqpp
  fmt
  ${Protobuf_LIBRARIES}
)

add_test(NAME UnitTests COMMAND ${CMAKE_PROJECT_NAME}_unittests)
