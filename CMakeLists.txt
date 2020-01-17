cmake_minimum_required(VERSION 3.15)
project(untitled11)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS -pthread)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -lpthread")

add_executable(server main.cpp TcpServer.cpp TcpServer.h DeadlineTimer.h DeadlineTimer.cpp Communist.h Communist.cpp
        timer.h timer.cpp Context.h Context.cpp)

add_executable(test test.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
