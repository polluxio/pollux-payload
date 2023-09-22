set (sources
  OptimizationFunctions.cpp
  Particle.cpp
  PolluxPayloadPSO.cpp
)

add_executable(pollux-payload-pso ${sources})

target_link_libraries(pollux-payload-pso pollux TBB::tbb)
