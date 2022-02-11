cmake_minimum_required(VERSION 3.18)
project(icon CXX)
set(CMAKE_CXX_STANDARD 20)

set(ICON_VERSION 0.1)

option(ENABLE_TESTING "Enable building and executing tests" OFF)

add_compile_options("-Wno-deprecated")

include(cmake/StaticAnalyzers.cmake)

# -----------------------------------------------------------------------
# Workaround for clang, Boost asio has relased coroutines without fully supported clang
# -----------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)


find_package(Protobuf REQUIRED)
include_directories(${protobuf_INCLUDES})
link_libraries(${protobuf_LIBRARIES})

set(PROTOBUF_GENERATE_CPP_APPEND_PATH FALSE)
protobuf_generate(
  IMPORT_DIRS ${CMAKE_SOURCE_DIR}
  LANGUAGE
    cpp
  OUT_VAR
    PROTO_SRCS
    PROTO_HDRS
  PROTOS
    ${CMAKE_SOURCE_DIR}/icon/metadata/metadata.proto
    ${CMAKE_SOURCE_DIR}/icon/protobuf/icon.proto
  PROTOC_OUT_DIR
    ${CMAKE_BINARY_DIR}/icon
)
add_library(proto-objlib OBJECT ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(proto-objlib PUBLIC ${CMAKE_BINARY_DIR}/icon)
install(DIRECTORY ${CMAKE_BINARY_DIR}/icon/icon DESTINATION include
  FILES_MATCHING PATTERN "*pb.h"
  PATTERN "CMakeFiles*" EXCLUDE
)



add_subdirectory(icon)

if (ENABLE_TESTING)
  include_directories(${CMAKE_BINARY_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif()
