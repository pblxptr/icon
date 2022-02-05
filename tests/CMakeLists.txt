

#Workaround for clang, Boost asio has relased coroutines without fully supported clang

find_package(Catch2 3 REQUIRED)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()

set (Protobuf_IMPORT_DIRS
  ${CMAKE_SOURCE_DIR}/icon/protobuf
  ${CMAKE_SOURCE_DIR}/tests
)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  ../icon/protobuf/metadata.proto
  ../icon/protobuf/icon.proto
  ./dummy.proto
)

set(TEST_LIBRARIES
  pthread
  icon
  protobuf
  zmq
  sodium
  spdlog
  fmt
  Catch2::Catch2WithMain
)

### UNIT TESTS ###
add_executable(${CMAKE_PROJECT_NAME}_unittests
  client/basic_client_tests.cpp
  client/request_tests.cpp
  client/response_tests.cpp
  utils/async_mutex_tests.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

add_dependencies(${CMAKE_PROJECT_NAME}_unittests ${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME}_unittests PRIVATE
  ${TEST_LIBRARIES}
)

add_test(NAME UnitTests COMMAND ${CMAKE_PROJECT_NAME}_unittests)

### INTEGRATION TESTS ###
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

