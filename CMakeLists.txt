cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

project(HellbenderSamples CXX)
include_directories(".")

set(HBE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/out)

add_subdirectory("HBE/")
add_subdirectory("Samples/Cube/")
add_subdirectory("Samples/Triangle/")
add_subdirectory("Samples/Model/")
add_subdirectory("Samples/Text/")
add_subdirectory("Samples/Instancing/")
add_subdirectory("Samples/Pong/")
add_subdirectory("Samples/Raytracer/")
add_subdirectory("Samples/RasterizationOrdered/")