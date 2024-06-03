# Указание исходных файлов
set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_client_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
)

# Создание исполняемых файлов
add_executable(server ${SERVER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/../generated/proto_example.pb.cc)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/common/include Threads::Threads ${Boost_LIBRARIES} ${CMAKE_CURRENT_BINARY_DIR}/../generated)

# Линковка с protobuf и Boost
target_link_libraries(server proto_files ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)
