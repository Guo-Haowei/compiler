cmake_minimum_required(VERSION 3.0)
project(minic C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_subdirectory(src)
add_subdirectory(unit-test)