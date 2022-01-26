

#Workaround for clang, Boost asio has relased coroutines without fully supported clang

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()

set (Protobuf_IMPORT_DIRS
  ${CMAKE_SOURCE_DIR}/src/protobuf
  ${CMAKE_SOURCE_DIR}/tests
)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  ../src/protobuf/metadata.proto
  ../src/protobuf/icon.proto
  ./dummy.proto
)


# set(APP_SOURCES
#   ../src/endpoint/basic_endpoint/basic_endpoint.cpp
# )

set(TEST_SOURCES
  catch.cpp
  send_receive_tests.cpp
  client/basic_client_tests.cpp
  client/request_tests.cpp
  client/response_tests.cpp
  utils/async_mutex_tests.cpp
)

# include_directories(../src/)

add_executable(${CMAKE_PROJECT_NAME}_unittests
  ${APP_SOURCES}
  ${TEST_SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

add_executable(${CMAKE_PROJECT_NAME}_e2e
  e2e/exchange_messages.cpp
)

add_dependencies(${CMAKE_PROJECT_NAME}_unittests ${CMAKE_PROJECT_NAME})
add_dependencies(${CMAKE_PROJECT_NAME}_e2e ${CMAKE_PROJECT_NAME})

target_link_libraries(${CMAKE_PROJECT_NAME}_unittests
  pthread
  icon
  CONAN_PKG::fmt
  CONAN_PKG::cppzmq
  CONAN_PKG::protobuf# ${Protobuf_LIBRARIES}
  CONAN_PKG::catch2
)

target_link_libraries(${CMAKE_PROJECT_NAME}_e2e
  pthread
  icon
  CONAN_PKG::fmt
  CONAN_PKG::cppzmq
  CONAN_PKG::protobuf# ${Protobuf_LIBRARIES}
  CONAN_PKG::catch2
)
add_test(NAME UnitTests COMMAND ${CMAKE_PROJECT_NAME}_unittests)
add_test(NAME BasicE2E COMMAND ${CMAKE_PROJECT_NAME}_e2e)

#CONAN_PKG
