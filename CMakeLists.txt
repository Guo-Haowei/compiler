cmake_minimum_required(VERSION 3.0)
project(cc C)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_subdirectory(src)