#[=======================================================================[.rst:
FindIoUring
-----------

Find the liburing library and headers.

This module defines the following ``IMPORTED`` target:

``IoUring::IoUring``
  The liburing library, if found.

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``IoUring_FOUND``
  True if liburing was found on the system
``IoUring_INCLUDE_DIRS``
  The dir to include to use liburing
``IoUring_LIBRARIES``
  The libraries to link against to use liburing
``IoUring_VERSION``
  The version of liburing found (if available)

#]=======================================================================]

find_path(IoUring_INCLUDE_DIR
  NAMES liburing.h
  PATHS /usr/include /usr/local/include /opt/local/include
  DOC "The directory where liburing.h resides"
)

find_library(IoUring_LIBRARY
  NAMES uring
  PATHS /usr/lib /usr/local/lib /opt/local/lib
  DOC "The liburing library"
)

# Handle version
if(IoUring_INCLUDE_DIR AND EXISTS "${IoUring_INCLUDE_DIR}/liburing.h")
  file(STRINGS "${IoUring_INCLUDE_DIR}/liburing.h" _uring_version_line
    REGEX "^#define.*IO_URING_VERSION_MAJOR"
  )
  if(_uring_version_line)
    string(REGEX MATCH "[0-9]+" _uring_major "${_uring_version_line}")
    file(STRINGS "${IoUring_INCLUDE_DIR}/liburing.h" _uring_minor_line
      REGEX "^#define.*IO_URING_VERSION_MINOR"
    )
    string(REGEX MATCH "[0-9]+" _uring_minor "${_uring_minor_line}")
    set(IoUring_VERSION "${_uring_major}.${_uring_minor}")
  endif()
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IoUring
  REQUIRED_VARS IoUring_LIBRARY IoUring_INCLUDE_DIR
  VERSION_VAR IoUring_VERSION
)

if(IoUring_FOUND)
  set(IoUring_INCLUDE_DIRS "${IoUring_INCLUDE_DIR}")
  set(IoUring_LIBRARIES "${IoUring_LIBRARY}")

  # Create imported target
  if(NOT TARGET IoUring::IoUring)
    add_library(IoUring::IoUring UNKNOWN IMPORTED)
    set_target_properties(IoUring::IoUring PROPERTIES
      IMPORTED_LOCATION "${IoUring_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${IoUring_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(IoUring_INCLUDE_DIR IoUring_LIBRARY)
