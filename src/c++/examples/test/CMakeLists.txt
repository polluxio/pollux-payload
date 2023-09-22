set (sources
  PolluxPayloadTest.cpp
)

add_executable(pollux-payload-test ${sources})

target_link_libraries(pollux-payload-test pollux)