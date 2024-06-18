# third-party/moodycamel/CMakeLists.txt

add_library(moodycamel INTERFACE)
target_include_directories(moodycamel INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

#add_subdirectory(spdlog)
