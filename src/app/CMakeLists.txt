add_executable(snl_app NajApp.cpp)

target_include_directories(snl_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(snl_app escher Naja::Naja)
