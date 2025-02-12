set (sources
  ZebulonPayloadClient.cpp 
  PolluxMethods.cpp
  PolluxPayload.cpp
)

add_library(pollux ${sources})

target_link_libraries(pollux pollux_grpc gpr spdlog::spdlog argparse)
target_include_directories(pollux PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
