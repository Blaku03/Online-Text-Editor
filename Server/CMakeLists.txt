# if you want to build server with cmake:
#> mkdir cmake-build
#> cd cmake-build
#> cmake ..
#> cmake --build .
# server executable will be inside cmake-build dir

# in root directory
#> cmake -B cmake-build -S .
#> cd cmake-build
#> make
#> make test

# in cmake-build
#> cmake -B . -S ..
#> make
#> make test

cmake_minimum_required(VERSION 3.22)
project(Server LANGUAGES C)

set(CMAKE_C_STANDARD 11)

enable_testing()

add_library(linked_list
        client_handler.c
        linked_list.c)

add_executable(server socket_server.c)
target_link_libraries(server PRIVATE linked_list)

if(BUILD_TESTING)
        add_subdirectory(test)
endif()
