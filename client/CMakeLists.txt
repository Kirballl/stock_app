# Указание исхоных файлов
set(CLIENT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
)

# Создание исполняемых файлов
add_executable(client ${CLIENT_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/../generated/proto_example.pb.cc)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/common/include Threads::Threads ${Boost_LIBRARIES} ${CMAKE_CURRENT_BINARY_DIR}/../generated)

# Линковка с protobuf и Boost
target_link_libraries(client proto_files ${PROTOBUF_LIBRARY} ${Boost_LIBRARIES} Threads::Threads)
