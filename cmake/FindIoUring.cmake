find_package(PkgConfig REQUIRED)

pkg_check_modules(PKG_URING REQUIRED liburing)

# additionally check if required files "really" exists
find_library(
        LIB_URING_LIB
        NAMES ${PKG_URING_LIBRARIES}
        HINTS ${PKG_URING_LIBDIR})

find_path(
        LIB_URING_INCLUDE
        NAMES liburing.h
        HINTS ${PKG_URING_INCLUDEDIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(IoUring DEFAULT_MSG
        LIB_URING_LIB
        PKG_URING_VERSION
        LIB_URING_INCLUDE)

mark_as_advanced(LIB_URING_LIB LIB_URING_INCLUDE)

if (IoUring_FOUND AND NOT TARGET IoUring::IoUring)
    add_library(IoUring::IoUring UNKNOWN IMPORTED)
    set_target_properties(
            IoUring::IoUring
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES ${LIB_URING_INCLUDE}
                IMPORTED_LOCATION ${LIB_URING_LIB})
endif()
