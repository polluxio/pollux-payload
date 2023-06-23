add_subdirectory(thirdparty)

set (sources
  common/ZebulonPayloadClient.cpp
  PolluxPayload.cpp
)

add_executable(polluxapp ${sources})

target_include_directories(polluxapp PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_link_libraries(polluxapp pollux_grpc spdlog::spdlog argparse)
