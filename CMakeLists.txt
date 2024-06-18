CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(my_stock)

SET(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.40 COMPONENTS system REQUIRED)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

include_directories(${Boost_INCLUDE_DIR} 
                    ${Protobuf_INCLUDE_DIRS} 
                    ${PQXX_INCLUDE_DIRS})

set(TARGET_LOGGER spdlog)

add_subdirectory(config)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(common)
add_subdirectory(proto)
add_subdirectory(third-party/spdlog)
add_subdirectory(third-party/moodycamel)
