CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(my_stock)

SET(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.40 COMPONENTS system REQUIRED)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(GTest REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(PostgreSQL REQUIRED)
find_package(PQXX REQUIRED)

include_directories(${Boost_INCLUDE_DIR} 
                    ${Protobuf_INCLUDE_DIRS} 
                    ${PQXX_INCLUDE_DIRS}
                    ${PostgreSQL_INCLUDE_DIRS}
                    ${OPENSSL_INCLUDE_DIR}
                    ${GTest_INCLUDE_DIRS}
                    third-party/jwt-cpp)

set(TARGET_LOGGER spdlog)

enable_testing()

add_subdirectory(config)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(common)
add_subdirectory(proto)
add_subdirectory(tests)
add_subdirectory(third-party/spdlog)
add_subdirectory(third-party/moodycamel)
add_subdirectory(third-party/jwt-cpp)
add_subdirectory(third-party/bcrypt)
