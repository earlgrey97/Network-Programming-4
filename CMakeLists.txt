# Minimum CMake required
cmake_minimum_required(VERSION 3.15)

# Project
project(20F_EE324_Assign4)

set(CMAKE_CXX_STANDARD 11)

# Protobuf
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

# Protobuf-compiler
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Proto file
get_filename_component(hw_proto "assign4.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/assign4.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/assign4.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/assign4.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/assign4.grpc.pb.h")

add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# supchild proto file
get_filename_component(hw_proto2 "supchild.proto" ABSOLUTE)
get_filename_component(hw_proto_path2 "${hw_proto2}" PATH)

set(hw_proto_srcs2 "${CMAKE_CURRENT_BINARY_DIR}/supchild.pb.cc")
set(hw_proto_hdrs2 "${CMAKE_CURRENT_BINARY_DIR}/supchild.pb.h")
set(hw_grpc_srcs2 "${CMAKE_CURRENT_BINARY_DIR}/supchild.grpc.pb.cc")
set(hw_grpc_hdrs2 "${CMAKE_CURRENT_BINARY_DIR}/supchild.grpc.pb.h")

add_custom_command(
      OUTPUT "${hw_proto_srcs2}" "${hw_proto_hdrs2}" "${hw_grpc_srcs2}" "${hw_grpc_hdrs2}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path2}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto2}"
      DEPENDS "${hw_proto2}")

# supsup proto file
get_filename_component(hw_proto3 "supsup.proto" ABSOLUTE)
get_filename_component(hw_proto_path3 "${hw_proto3}" PATH)

set(hw_proto_srcs3 "${CMAKE_CURRENT_BINARY_DIR}/supsup.pb.cc")
set(hw_proto_hdrs3 "${CMAKE_CURRENT_BINARY_DIR}/supsup.pb.h")
set(hw_grpc_srcs3 "${CMAKE_CURRENT_BINARY_DIR}/supsup.grpc.pb.cc")
set(hw_grpc_hdrs3 "${CMAKE_CURRENT_BINARY_DIR}/supsup.grpc.pb.h")

add_custom_command(
      OUTPUT "${hw_proto_srcs3}" "${hw_proto_hdrs3}" "${hw_grpc_srcs3}" "${hw_grpc_hdrs3}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path3}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto3}"
      DEPENDS "${hw_proto3}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

#ADD_COMPILE_OPTIONS(-std=c++11)

# Targets (client|server)
foreach(_target
  client super child)
  add_executable(${_target} "${_target}.cc"
    ${hw_proto_srcs}
    ${hw_grpc_srcs} ${hw_proto_srcs2} ${hw_grpc_srcs2} ${hw_proto_srcs3} ${hw_grpc_srcs3})
  target_link_libraries(${_target}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
