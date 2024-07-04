# server/CMakeLists.txt

set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/order_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client_data_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_client_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/auth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/time_order_utils.cpp
)

add_library(server_lib ${SERVER_SOURCES})
                      
target_link_libraries(server_lib ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} ${TARGET_LOGGER}
                             ${PostgreSQL_LIBRARIES} pqxx ${OPENSSL_LIBRARIES}
                             Threads::Threads proto_files moodycamel config common jwt-cpp bcrypt)

target_include_directories(server_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(server ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(server server_lib)

add_custom_command(
    TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../config/server_config.ini
        $<TARGET_FILE_DIR:server>
)
