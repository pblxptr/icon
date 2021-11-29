cmake_minimum_required(VERSION 3.14)
project(zmq_boost)

find_package(Boost REQUIRED)

set(SOURCES
  main.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME} pthread zmq zmqpp)
