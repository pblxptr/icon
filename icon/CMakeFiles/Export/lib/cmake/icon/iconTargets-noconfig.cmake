#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "icon::icon" for configuration ""
set_property(TARGET icon::icon APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(icon::icon PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libicon.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS icon::icon )
list(APPEND _IMPORT_CHECK_FILES_FOR_icon::icon "${_IMPORT_PREFIX}/lib/libicon.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
