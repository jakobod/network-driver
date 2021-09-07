# -- project setup -------------------------------------------------------------

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(lib_net CXX)

include(FetchContent)

# -- set useful CMake options --------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_compile_options(-Wall -Wextra -pedantic)

# -- find or fetch dependencies ------------------------------------------------

# Threads
find_package(Threads)

# Googletest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

# -- project files -------------------------------------------------------------

# Find the packet-routing headers
file(GLOB_RECURSE LIB_NET_HEADERS "header/*.hpp")

# New source files have to be added here
set(LIB_NET_SOURCES
  src/detail/error_code.cpp
  src/net/acceptor.cpp
  src/net/operation.cpp
  src/net/epoll_multiplexer.cpp
  src/net/pipe_socket.cpp
  src/net/pollset_updater.cpp
  src/net/raw_socket.cpp
  src/net/socket_manager.cpp
  src/net/socket.cpp
  src/net/stream_socket.cpp
  src/net/tcp_accept_socket.cpp
  src/net/tcp_stream_socket.cpp
  src/packet/packet.cpp
  src/packet/payload.cpp
)

# -- Object for different targets ----------------------------------------------

add_library(lib_net_obj OBJECT
            ${LIB_NET_HEADERS} ${LIB_NET_SOURCES})
set_property(TARGET lib_net_obj PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(lib_net_obj PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/header")
target_link_libraries(lib_net_obj Threads::Threads)

# -- add executables -----------------------------------------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

macro(add_target name)
  add_executable(${name} "src/${name}.cpp")
  target_include_directories(${name} PRIVATE "header")
  target_link_libraries(${name} PRIVATE lib_net_obj)
endmacro()

# add_target(main) # This is the main project file

# -- test setup ----------------------------------------------------------------

enable_testing()

add_executable(
  lib_net_test
  test/acceptor.cpp
  test/tcp_stream_socket.cpp
)

target_include_directories(lib_net_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/header")

target_link_libraries(
  lib_net_test
  lib_net_obj
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(lib_net_test)
