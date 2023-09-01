set (sources
  PolluxPayload.cpp
)

add_executable(pollux-payload-example ${sources})

target_link_libraries(pollux-payload-example pollux)