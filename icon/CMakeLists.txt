# -----------------------------------------------------------------------
# Setup external packages
# -----------------------------------------------------------------------
find_package(spdlog REQUIRED)
find_package(Boost 1.77 REQUIRED coroutine)
find_package(Protobuf REQUIRED)

# -----------------------------------------------------------------------
# Generate proto files
# -----------------------------------------------------------------------
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

# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
#   ${CMAKE_SOURCE_DIR}/icon/metadata/metadata.proto
#   ${CMAKE_SOURCE_DIR}/icon/protobuf/icon.proto
# )

# -----------------------------------------------------------------------
# Make static library from objlib
# -----------------------------------------------------------------------
set(SOURCES endpoint/basic_endpoint/basic_endpoint.cpp)
add_library(objlib OBJECT
  ${SOURCES}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

message(WARNING "CMAKE_CURRENT_BINARY_DIR " ${CMAKE_CURRENT_BINARY_DIR})
message(WARNING "CMAKE_BINARY_DIR " ${CMAKE_BINARY_DIR})

target_include_directories(objlib PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(objlib PRIVATE ${CMAKE_BINARY_DIR}/icon)
target_link_libraries(objlib PRIVATE
  lpthread
  spdlog::spdlog
)
add_library(${CMAKE_PROJECT_NAME} STATIC $<TARGET_OBJECTS:objlib>)

# -----------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------
install(DIRECTORY ${CMAKE_SOURCE_DIR}/icon DESTINATION include
  FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.proto"
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/icon DESTINATION include
  FILES_MATCHING PATTERN "*pb.h"
  PATTERN "CMakeFiles*" EXCLUDE
)

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