set(PROTO_FILES escher_payload.proto)

add_library(escher STATIC ${PROTO_FILES})
target_link_libraries(escher PUBLIC
  protobuf::libprotobuf
  gRPC::grpc
  gRPC::grpc++
)
target_include_directories(escher INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET escher LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=`which grpc_cpp_plugin`")
