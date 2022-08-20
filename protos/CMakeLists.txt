# Find google's Protobuf and GRPC libraries.
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
set(PROTO_FILES messages.proto)

add_library(messages ${PROTO_FILES})
target_link_libraries(messages PUBLIC protobuf::libprotobuf gRPC::grpc gRPC::grpc++)

target_include_directories(messages PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET messages LANGUAGE cpp)
protobuf_generate(TARGET messages LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")