cmake_minimum_required(VERSION 3.15)
project(X)

set(CMAKE_CXX_STANDARD 14)

add_executable(X server.cpp client.cpp test.cpp)