# Указание исходных файлов
set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_client_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../config/config.cpp
)

# Создание исполняемых файлов
add_executable(server ${SERVER_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated/trade_market_protocol.pb.cc)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../config ${CMAKE_SOURCE_DIR}/common/include Threads::Threads ${Boost_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated)

# Линковка с protobuf и Boost
target_link_libraries(server proto_files ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)

# Копирование конфигурационного файла
add_custom_command(
    TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../config/server_config.ini
        $<TARGET_FILE_DIR:server>
)
