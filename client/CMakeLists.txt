# Указание исходных файлов
set(CLIENT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../config/config.cpp
)

# Создание исполняемых файлов
add_executable(client ${CLIENT_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated/trade_market_protocol.pb.cc)
target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../config ${CMAKE_SOURCE_DIR}/common/include Threads::Threads ${Boost_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../proto/generated)

# Линковка с protobuf и Boost
target_link_libraries(client proto_files ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)

# Копирование конфигурационного файла
add_custom_command(
    TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../config/server_config.ini
        $<TARGET_FILE_DIR:client>
)
