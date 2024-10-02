#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "PhysFS::PhysFS" for configuration "Release"
set_property(TARGET PhysFS::PhysFS APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(PhysFS::PhysFS PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/physfs.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/physfs.dll"
  )

list(APPEND _cmake_import_check_targets PhysFS::PhysFS )
list(APPEND _cmake_import_check_files_for_PhysFS::PhysFS "${_IMPORT_PREFIX}/lib/physfs.lib" "${_IMPORT_PREFIX}/bin/physfs.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
