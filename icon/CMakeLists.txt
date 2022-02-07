# -----------------------------------------------------------------------
# Setup external packages
# -----------------------------------------------------------------------
find_package(spdlog REQUIRED)
find_package(Boost 1.77 REQUIRED coroutine)
find_package(Protobuf REQUIRED)
set (Protobuf_IMPORT_DIRS ${CMAKE_SOURCE_DIR}/icon/protobuf)

# -----------------------------------------------------------------------
# Workaround for clang, Boost asio has relased coroutines without fully supported clang
# -----------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
  set(CMAKE_CXX_FLAGS "-std=c++20")
endif()
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)

# -----------------------------------------------------------------------
# Make static library from objlib
# -----------------------------------------------------------------------
set(SOURCES
  endpoint/basic_endpoint/basic_endpoint.cpp
)
protobuf_generate_cpp(
  PROTO_SRCS PROTO_HDRS
  protobuf/metadata.proto
  protobuf/icon.proto
)

add_library(objlib OBJECT
  ${SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

target_include_directories(objlib PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(objlib PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(objlib PRIVATE
  lpthread
  spdlog::spdlog
)
add_library(${CMAKE_PROJECT_NAME} STATIC $<TARGET_OBJECTS:objlib>)

# -----------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(FILES
  ${CMAKE_BINARY_DIR}/icon/metadata.pb.h
  ${CMAKE_BINARY_DIR}/icon/icon.pb.h
DESTINATION include/icon )

message(TEFERERE + ${CMAKE_BINARY_DIR})
install(TARGETS ${CMAKE_PROJECT_NAME} EXPORT iconTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# -----------------------------------------------------------------------
# Package configure
# -----------------------------------------------------------------------
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)

set(ConfigPackageLocation lib/cmake/icon)

generate_export_header(icon)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/icon/iconConfigVersion.cmake"
  VERSION ${ICON_VERSION}
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT iconTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/icon/iconTargets.cmake"
  NAMESPACE icon::
)
configure_file(${CMAKE_SOURCE_DIR}/cmake/iconConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/icon/iconConfig.cmake"
  COPYONLY
)

install(EXPORT iconTargets
  FILE
    iconTargets.cmake
  NAMESPACE
  icon::
  DESTINATION
    ${ConfigPackageLocation}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/icon/iconConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/icon/iconConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
)