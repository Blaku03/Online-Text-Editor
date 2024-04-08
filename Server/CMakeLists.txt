#if you want to build server with cmake:
#mkdir cmake-build
#cd cmake-build
#cmake --build .
#server executable will inside cmake-build dir

cmake_minimum_required(VERSION 3.22)
project(Server C)

set(CMAKE_C_STANDARD 11)

add_executable(Server
        socket_server.c)
