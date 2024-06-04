CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(my_stock)

SET(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.40 COMPONENTS system REQUIRED)
find_package(Threads REQUIRED)

# Указание пути к библиотекам и заголовочным файлам protobuf
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
link_directories(${Protobuf_LIBRARY_DIRS})

find_program(PROTOC_EXECUTABLE protoc)
if (NOT PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc executable not found")
endif()

# Обработка .proto файлов
set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/proto_example.proto)
set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

# Команда для компилляции .proto файлов 
add_custom_command(
    OUTPUT ${GENERATED_PROTO_DIR}/proto_example.pb.cc ${GENERATED_PROTO_DIR}/proto_example.pb.h
    COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${GENERATED_PROTO_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "Running protoc on ${PROTO_FILES}"
)

# Добавление сгенерированных файлов в библиотеку
add_library(proto_files ${GENERATED_PROTO_DIR}/proto_example.pb.cc)

# Указание директорий с заголовочными файлами
include_directories(${Boost_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/common/include ${GENERATED_PROTO_DIR})

# Добавление подкаталога 
add_subdirectory(server)
add_subdirectory(client)
