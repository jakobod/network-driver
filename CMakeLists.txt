# -- project setup -------------------------------------------------------------

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(lib_net CXX)

include(FetchContent)
include(GoogleTest)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

# -- set useful CMake options --------------------------------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_compile_options(-Wall -Wextra -pedantic)

message (STATUS "Build type ${CMAKE_BUILD_TYPE}")

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

# Threads
find_package(OpenSSL REQUIRED)

# -- project files -------------------------------------------------------------

# Find the packet-routing headers
file(GLOB_RECURSE LIB_NET_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/libnet/*.hpp")

# New source files have to be added here
set(LIB_NET_SOURCES
  src/net/acceptor.cpp
  src/net/datagram_socket.cpp
  src/net/event_result.cpp
  src/net/ip/v4_address.cpp
  src/net/ip/v4_endpoint.cpp
  src/net/multiplexer_impl.cpp
  src/net/operation.cpp
  src/net/pipe_socket.cpp
  src/net/pollset_updater.cpp
  src/net/socket_manager.cpp
  src/net/socket.cpp
  src/net/stream_socket.cpp
  src/net/tcp_accept_socket.cpp
  src/net/tcp_stream_socket.cpp
  src/net/udp_datagram_socket.cpp
  src/net/uri.cpp
  src/openssl/tls_context.cpp
  src/openssl/tls_session.cpp
  src/util/binary_deserializer.cpp
  src/util/binary_serializer.cpp
  src/util/cli_parser.cpp
  src/util/error_code.cpp
  src/util/error.cpp
  src/util/format.cpp
)

# -- Object for different targets ----------------------------------------------

add_library(net STATIC ${LIB_NET_HEADERS} ${LIB_NET_SOURCES})
set_property(TARGET net PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(net PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libnet")
target_link_libraries(net Threads::Threads OpenSSL::Crypto OpenSSL::SSL)

# Copy Dummy certs to build folder and set preprocessor definition to the folder
file(COPY "${PROJECT_SOURCE_DIR}/certs" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
add_compile_definitions(CERT_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}/certs")

# -- logging configuration -----------------------------------------------------

if(LIB_NET_LOG_LEVEL)
  message(STATUS "Logging enabled: ${LIB_NET_LOG_LEVEL}")
  add_compile_definitions(NET_LOG_LEVEL=${LIB_NET_LOG_LEVEL})
else()
  message(STATUS "Logging disabled")
endif()

# -- main for playing with things ----------------------------------------------

macro(add_target name)
  add_executable(${name} "src/${name}.cpp")
  target_include_directories(${name} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/libnet")
  target_link_libraries(${name} PRIVATE net)
endmacro()

add_target(playground)

# -- test setup ----------------------------------------------------------------

if (LIB_NET_ENABLE_TESTS)
  enable_testing()

  add_executable(
    lib_net_test
    test/net_test_main.cpp
    test/net/acceptor.cpp
    test/net/datagram_socket.cpp
    test/net/ip/v4_address.cpp
    test/net/ip/v4_endpoint.cpp
    test/net/multiplexer_impl.cpp
    test/net/pollset_updater.cpp
    test/net/socket_guard.cpp
    test/net/socket_manager.cpp
    test/net/stream_transport.cpp
    test/net/tcp_socket.cpp
    test/net/tls.cpp
    test/net/transport_adaptor.cpp
    test/net/udp_datagram_socket.cpp
    test/net/uri.cpp
    test/openssl/communication.cpp
    test/util/binary_deserializer.cpp
    test/util/binary_serializer.cpp
    test/util/cli_parser.cpp
    test/util/format.cpp
    test/util/intrusive_ptr.cpp
    test/util/ref_counted.cpp
    test/util/scope_guard.cpp
    test/util/serialized_size.cpp
  )

  target_include_directories(lib_net_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libnet")
  target_include_directories(lib_net_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/test")

  target_link_libraries(
    lib_net_test
    net
    gtest
  )

  gtest_discover_tests(lib_net_test)
endif()
