# tests/CMakeLists.txt

set(TEST_SOURCES
    main_test.cpp
    core_test.cpp)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../server
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks  
)

add_executable(trade_tests ${TEST_SOURCES})

target_link_libraries(trade_tests
    GTest::GTest
    GTest::Main
    GTest::gmock
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GMOCK_MAIN_LIBRARIES} 
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    ${PQXX_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    server_lib
    Threads::Threads
    proto_files
    config
    common
    spdlog
    moodycamel
    jwt-cpp 
    bcrypt
)

add_test(NAME runTests COMMAND trade_tests)

include(GoogleTest)
