set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/order_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_client_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../config/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated/trade_market_protocol.pb.cc
)

add_executable(server ${SERVER_SOURCES})
                      
# Линковка с protobuf и Boost и spdlog и прочими
target_link_libraries(server ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} ${TARGET_LOGGER}
                             proto_files Threads::Threads moodycamel config common)

# Копирование конфигурационного файла
add_custom_command(
    TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../config/server_config.ini
        $<TARGET_FILE_DIR:server>
)
