# tests/CMakeLists.txt

set(TEST_SOURCES
    main_test.cpp
    core_test.cpp)

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
