
set (Protobuf_IMPORT_DIRS ${CMAKE_SOURCE_DIR}/icon/protobuf)
protobuf_generate_cpp(
  PROTO_SRCS PROTO_HDRS
  protobuf/metadata.proto
  protobuf/icon.proto
)

#Workaround for clang, Boost asio has relased coroutines without fully supported clang

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()

set(SOURCES
  endpoint/basic_endpoint/basic_endpoint.cpp
)
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)

include_directories(${CMAKE_SOURCE_DIR})

set(STATIC_LIB_NAME ${CMAKE_PROJECT_NAME})

add_library(objlib OBJECT
  ${SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

# Make static library
add_library(${STATIC_LIB_NAME} STATIC $<TARGET_OBJECTS:objlib>)

# target_link_libraries(${STATIC_LIB_NAME}
#   pthread
#   cppzmq
#   spdlog
#   ${Protobuf_LIBRARIES}
# )
