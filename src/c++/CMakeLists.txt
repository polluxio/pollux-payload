add_subdirectory(thirdparty)

add_subdirectory(pollux)

set (sources
  PolluxPayload.cpp
)

add_executable(polluxapp ${sources})

target_link_libraries(polluxapp pollux)
