cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.22)

project(polluxapp)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(thirdparty)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)
find_package(TBB) #for PSO

add_subdirectory(src)
