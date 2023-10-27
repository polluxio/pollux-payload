set (sources
	PolluxPayloadNativeC.cpp
)

add_executable(pollux-payload-nativeC ${sources})

target_link_libraries(pollux-payload-nativeC pollux)
