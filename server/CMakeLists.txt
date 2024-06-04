# Указание исходных файлов
set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_client_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
)

# Создание исполняемых файлов
add_executable(server ${SERVER_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated/trade_market_protocol.pb.cc)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/common/include Threads::Threads ${Boost_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated)

# Линковка с protobuf и Boost
target_link_libraries(server proto_files ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)
