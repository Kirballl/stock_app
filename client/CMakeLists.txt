set(CLIENT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../config/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated/trade_market_protocol.pb.cc
)

add_executable(client ${CLIENT_SOURCES})

# Линковка с protobuf и Boost и spdlog и других
target_link_libraries(client ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} ${TARGET_LOGGER}
                               Threads::Threads proto_files config common)

# Копирование конфигурационного файла
add_custom_command(
    TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../config/server_config.ini
        $<TARGET_FILE_DIR:client>
)
