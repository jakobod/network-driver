cmake_minimum_required(VERSION 3.17)
project(raw_sockets)

set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(header)

set(SRC_FILES
        src/packet.cpp
        src/raw_socket.cpp
        src/ethernet.cpp
        src/payload.cpp
        src/pipe_socket.cpp
        src/socket.cpp)

find_package(Threads REQUIRED)

add_executable(raw_sockets
        main.cpp
        ${SRC_FILES})

target_link_libraries(raw_sockets
        ${CMAKE_THREAD_LIBS_INIT})