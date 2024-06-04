# Указание исходных файлов
set(CLIENT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
)

# Создание исполняемых файлов
add_executable(client ${CLIENT_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated/trade_market_protocol.pb.cc)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/common/include Threads::Threads ${Boost_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated)

# Линковка с protobuf и Boost
target_link_libraries(client proto_files ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)
