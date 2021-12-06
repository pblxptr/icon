cmake_minimum_required(VERSION 3.14)
project(zmq_boost)

find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
  main.cpp
)
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)


add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME} pthread zmq zmqpp)
