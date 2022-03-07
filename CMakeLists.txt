cmake_policy(SET CMP0048 NEW)

project(escherapp)

cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

add_subdirectory(src)
