cmake_minimum_required(VERSION 3.17)
project(lmcomm
LANGUAGES CXX)


set(BUILD_LMCOMM_TEST false)
set(BUILD_LMCOMM_WITH_DEBUG true)


if(BUILD_LMCOMM_WITH_DEBUG)
    message("####################### build width debug #######################")
    add_compile_options(-g3 -O0)
endif()

add_library(${CMAKE_PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/net/socket_utils.cpp
    ${CMAKE_SOURCE_DIR}/poller/event_poller.cpp
    ${CMAKE_SOURCE_DIR}/poller/pipe_wrapper.cpp
    ${CMAKE_SOURCE_DIR}/utils/logger.cpp 
    ${CMAKE_SOURCE_DIR}/utils/utils.cpp 
    ${CMAKE_SOURCE_DIR}/utils/uv_error.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_11)
target_link_libraries(${CMAKE_PROJECT_NAME} pthread)

if(BUILD_LMCOMM_TEST)
    message("####################### build test #######################")
    add_executable(test_thread_pool 
        ${CMAKE_SOURCE_DIR}/test/test_thread_pool.cpp
    )
    add_dependencies(test_thread_pool 
        ${CMAKE_PROJECT_NAME}
    )
    target_include_directories(test_thread_pool PUBLIC ${CMAKE_SOURCE_DIR})
    target_compile_features(test_thread_pool PUBLIC cxx_std_11)
    target_link_libraries(test_thread_pool ${CMAKE_PROJECT_NAME} pthread)


    add_executable(test_event_poller 
        ${CMAKE_SOURCE_DIR}/test/test_event_poller.cpp
    )
    add_dependencies(test_event_poller 
        ${CMAKE_PROJECT_NAME}
    )
    target_include_directories(test_event_poller PUBLIC ${CMAKE_SOURCE_DIR})
    target_compile_features(test_event_poller PUBLIC cxx_std_11)
    target_link_libraries(test_event_poller ${CMAKE_PROJECT_NAME} pthread)
endif()