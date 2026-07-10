#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ZBase::zbase" for configuration "Debug"
set_property(TARGET ZBase::zbase APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(ZBase::zbase PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/zbase.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/zbase.dll"
  )

list(APPEND _cmake_import_check_targets ZBase::zbase )
list(APPEND _cmake_import_check_files_for_ZBase::zbase "${_IMPORT_PREFIX}/lib/zbase.lib" "${_IMPORT_PREFIX}/bin/zbase.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
