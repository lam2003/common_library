cmake_minimum_required(VERSION 3.17)
add_executable(test ${CMAKE_SOURCE_DIR}/main.cpp ${CMAKE_SOURCE_DIR}/utils/utils.cpp)

target_include_directories(test PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(test PUBLIC cxx_std_11)
target_link_libraries(test pthread)
