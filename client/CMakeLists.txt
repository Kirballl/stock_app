# client/CMakeLists.txt

set(CLIENT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/user_interface.cpp
)

add_executable(client ${CLIENT_SOURCES})

target_link_libraries(client ${Protobuf_LIBRARIES} ${Boost_LIBRARIES} ${TARGET_LOGGER}
                               Threads::Threads proto_files config common)

add_custom_command(
    TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../config/server_config.ini
        $<TARGET_FILE_DIR:client>
)
