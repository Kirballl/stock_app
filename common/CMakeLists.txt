# common/CMakeLists.txt

add_library(common STATIC common.cpp)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(common PUBLIC config spdlog)
