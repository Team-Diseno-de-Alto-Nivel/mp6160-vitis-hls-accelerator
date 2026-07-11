cmake_minimum_required(VERSION 3.16)
project(accel_driver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(driver
    src/main.c
)
