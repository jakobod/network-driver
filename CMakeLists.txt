cmake_minimum_required(VERSION 3.17)
project(epoll_benchmark)

set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(header)

set(SRC_FILES
    src/net/acceptor.cpp
    src/net/multiplexer.cpp
    src/net/pipe_socket.cpp
    src/net/pollset_updater.cpp
    src/net/raw_socket.cpp
    src/net/socket_manager_impl.cpp
    src/net/socket_manager.cpp
    src/net/socket.cpp
    src/net/stream_socket.cpp
    src/net/tcp_accept_socket.cpp
    src/net/tcp_stream_socket.cpp
    src/packet/packet.cpp
    src/packet/payload.cpp)

find_package(Threads REQUIRED)

add_executable(epoll_benchmark
               main.cpp
               ${SRC_FILES})

target_link_libraries(epoll_benchmark
                      ${CMAKE_THREAD_LIBS_INIT})