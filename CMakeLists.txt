cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.22)

project(polluxapp)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
#fetch argparse v2.9
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG 997da9255618311d1fcb0135ce86022729d1f1cb
)
FetchContent_MakeAvailable(argparse)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

add_subdirectory(src)