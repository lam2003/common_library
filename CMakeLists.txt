cmake_minimum_required(VERSION 3.17)

add_compile_options(-g)

add_executable(test ${CMAKE_SOURCE_DIR}/main.cpp ${CMAKE_SOURCE_DIR}/utils/utils.cpp ${CMAKE_SOURCE_DIR}/poller/event_poller.cpp ${CMAKE_SOURCE_DIR}/net/socket_utils.cpp ${CMAKE_SOURCE_DIR}/utils/logger.cpp 
${CMAKE_SOURCE_DIR}/poller/pipe_wrapper.cpp
)
target_include_directories(test PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(test PUBLIC cxx_std_11)
target_link_libraries(test pthread)


add_executable(test_thread_pool ${CMAKE_SOURCE_DIR}/test/test_thread_pool.cpp ${CMAKE_SOURCE_DIR}/utils/utils.cpp ${CMAKE_SOURCE_DIR}/utils/logger.cpp )
target_include_directories(test_thread_pool PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(test_thread_pool PUBLIC cxx_std_11)
target_link_libraries(test_thread_pool pthread)
