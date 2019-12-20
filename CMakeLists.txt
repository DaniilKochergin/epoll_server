cmake_minimum_required(VERSION 3.15)
project(untitled11)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS -pthread)

add_executable(untitled11 main.cpp TcpServer.cpp TcpServer.h DeadlineTimer.h DeadlineTimer.cpp Communist.h Communist.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")