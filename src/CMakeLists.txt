
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  protobuf/metadata.proto
  protobuf/icon.proto
)

set(CMAKE_CXX_STANDARD 20)

#Workaround for clang, Boost asio has relased coroutines without fully supported clang


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
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


