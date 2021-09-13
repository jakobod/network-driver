# -- project setup -------------------------------------------------------------

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(lib_net CXX)

include(FetchContent)
include(GoogleTest)

# -- set useful CMake options --------------------------------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_compile_options(-Wall -Wextra -pedantic)

# -- find or fetch dependencies ------------------------------------------------

# Threads
find_package(Threads REQUIRED)

if (LIB_NET_ENABLE_TESTS)
  # Googletest
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  )
  FetchContent_MakeAvailable(googletest)
endif()

# -- project files -------------------------------------------------------------

# Find the packet-routing headers
file(GLOB_RECURSE LIB_NET_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/header/*.hpp")

# New source files have to be added here
set(LIB_NET_SOURCES
  src/net/acceptor.cpp
  src/net/operation.cpp
  src/net/epoll_multiplexer.cpp
  src/net/error_code.cpp
  src/net/pipe_socket.cpp
  src/net/pollset_updater.cpp
  src/net/raw_socket.cpp
  src/net/socket_manager.cpp
  src/net/socket.cpp
  src/net/stream_socket.cpp
  src/net/tcp_accept_socket.cpp
  src/net/tcp_stream_socket.cpp
)

# -- Object for different targets ----------------------------------------------

add_library(net STATIC ${LIB_NET_HEADERS} ${LIB_NET_SOURCES})
set_property(TARGET net PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(net PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/header")
target_link_libraries(net Threads::Threads)

# -- test setup ----------------------------------------------------------------

if (LIB_NET_ENABLE_TESTS)
  enable_testing()

  add_executable(
    lib_net_test
    test/net/socket_guard.cpp
    test/net/acceptor.cpp
    test/net/epoll_multiplexer.cpp
    test/net/stream_manager.cpp
    test/net/tcp_socket.cpp
  )

  target_include_directories(lib_net_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/header")
  target_include_directories(lib_net_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/test")

  target_link_libraries(
    lib_net_test
    net
    gtest_main
  )

  gtest_discover_tests(lib_net_test)
endif()
