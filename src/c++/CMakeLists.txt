add_executable(escher_app EscherApp.cpp)

target_link_libraries(escher_app escher_grpc)

#target_include_directories(snl_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
