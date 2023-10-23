# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.0)

# Set the project name and language
project(SquareRoot CXX)

# Set the C++ standard and compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall")

# Add an executable target with the source files
add_executable(main main.cpp pollux.h pollux_api.h)
add_library(pollux SHARED IMPORTED)
set_target_properties(pollux PROPERTIES IMPORTED_LOCATION ./pollux.so)
target_link_libraries(main pollux)
