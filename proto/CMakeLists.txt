# proto/CMakeLists.txt

CMAKE_MINIMUM_REQUIRED(VERSION 3.21)

find_package(Protobuf REQUIRED)
find_program(PROTOC_EXECUTABLE protoc)
if (NOT PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc executable not found")
endif()

set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/trade_market_protocol.proto)
set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

add_custom_command(
    OUTPUT ${GENERATED_PROTO_DIR}/trade_market_protocol.pb.cc ${GENERATED_PROTO_DIR}/trade_market_protocol.pb.h
    COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${GENERATED_PROTO_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "Running protoc on ${PROTO_FILES}"
)

add_library(proto_files ${GENERATED_PROTO_DIR}/trade_market_protocol.pb.cc)

target_include_directories (proto_files PUBLIC ${Protobuf_INCLUDE_DIRS} ${GENERATED_PROTO_DIR})

target_link_libraries(proto_files PUBLIC ${Protobuf_LIBRARIES})
