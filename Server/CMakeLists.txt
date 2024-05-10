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
#> ctest

cmake_minimum_required(VERSION 3.22)
project(Server LANGUAGES C)

set(CMAKE_C_STANDARD 11)

include(CTest)

add_library(linked_list
        client_handler.c
        linked_list.c)
target_compile_options(linked_list PRIVATE -Wall -Wextra)


add_executable(server socket_server.c)
target_link_libraries(server PRIVATE linked_list)
target_compile_options(server PRIVATE -Wall -Wextra)

if(BUILD_TESTING)
        add_subdirectory(test)
endif()
