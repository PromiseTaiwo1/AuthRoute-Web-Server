cmake_minimum_required(VERSION 3.22)
project(AuthRoute_Web_Server C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(AuthRoute_Web_Server
        main.c
        main.h)
