
cmake_minimum_required(VERSION 3.18)

# -----------------------------------------------------------------------
# Setup external packages
# -----------------------------------------------------------------------
find_package(Catch2 3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)

# -----------------------------------------------------------------------
# Generate protobuf
# -----------------------------------------------------------------------
set(Protobuf_IMPORT_DIRS ${CMAKE_SOURCE_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  ${CMAKE_SOURCE_DIR}/tests/dummy.proto
)

set(TEST_LIBRARIES
  pthread
  icon
  protobuf
  zmq
  sodium
  spdlog::spdlog
  fmt
  Catch2::Catch2WithMain
)

# -----------------------------------------------------------------------
# Build UnitTests executable
# -----------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/icon)

add_executable(${CMAKE_PROJECT_NAME}_unittests
  client/basic_client_tests.cpp
  client/request_tests.cpp
  client/response_tests.cpp
  utils/async_mutex_tests.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

add_dependencies(${CMAKE_PROJECT_NAME}_unittests ${CMAKE_PROJECT_NAME})
target_include_directories(${CMAKE_PROJECT_NAME}_unittests PRIVATE
  ${spdlog_INCLUDE_DIRS}
)
target_link_libraries(${CMAKE_PROJECT_NAME}_unittests PRIVATE
  ${TEST_LIBRARIES}
)
add_test(NAME UnitTests COMMAND ${CMAKE_PROJECT_NAME}_unittests)

# -----------------------------------------------------------------------
# Build IntegrationTests executable
# -----------------------------------------------------------------------
add_executable(${CMAKE_PROJECT_NAME}_e2e
  e2e/basic_exchange_messages_test.cpp
  e2e/extended_exchange_messages_test.cpp
  e2e/send_receive_tests.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)
add_dependencies(${CMAKE_PROJECT_NAME}_e2e ${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME}_e2e
  ${TEST_LIBRARIES}
)
add_test(NAME BasicE2E COMMAND ${CMAKE_PROJECT_NAME}_e2e)

