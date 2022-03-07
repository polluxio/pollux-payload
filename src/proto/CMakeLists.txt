set(PROTO_FILES escher.proto escher_payload.proto)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTO_FILES})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTO_FILES})

add_library(escher_grpc STATIC ${PROTO_SRCS} ${GRPC_SRCS})

target_link_libraries(escher_grpc PUBLIC
  protobuf::libprotobuf
  gRPC::grpc++_reflection
)

target_include_directories(escher_grpc INTERFACE ${PROTO_SRC_DIR})
