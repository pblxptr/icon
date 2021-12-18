cmake_minimum_required(VERSION 3.14)
project(zmq_boost)

find_package(Boost REQUIRED coroutine)
find_package(Protobuf REQUIRED)


protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS metadata.proto icon.proto)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
  main.cpp
)
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)


include_directories(build)


add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(${CMAKE_PROJECT_NAME} pthread zmq zmqpp fmt ${Protobuf_LIBRARIES})
