cmake_minimum_required(VERSION 3.16)

if(BUILD_LMCOMM_WITH_DEBUG)
    message("####################### build width debug #######################")
    add_compile_options(-g3 -O0)
endif()

add_library(lmcomm
    ${CMAKE_CURRENT_SOURCE_DIR}/net/socket_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/net/socket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/net/dns_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/poller/event_poller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/poller/pipe_wrapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/poller/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/logger.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/uv_error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread/worker.cpp
)

target_include_directories(lmcomm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(lmcomm PUBLIC cxx_std_11)
target_compile_options(lmcomm PUBLIC -Wall)

if(BUILD_LMCOMM_TEST)
    message("####################### build test #######################")
    add_executable(test_thread_pool 
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_thread_pool.cpp
    )
    add_dependencies(test_thread_pool 
        lmcomm
    )
    target_include_directories(test_thread_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_thread_pool PUBLIC cxx_std_11)
    target_link_libraries(test_thread_pool lmcomm pthread)


    add_executable(test_event_poller 
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_event_poller.cpp
    )
    add_dependencies(test_event_poller 
        lmcomm
    )
    target_include_directories(test_event_poller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_event_poller PUBLIC cxx_std_11)
    target_link_libraries(test_event_poller lmcomm pthread)

    
    add_executable(test_dns_cache
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_dns_cache.cpp
    )
    add_dependencies(test_dns_cache 
        lmcomm
    )
    target_include_directories(test_dns_cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_dns_cache PUBLIC cxx_std_11)
    target_link_libraries(test_dns_cache lmcomm pthread)

    add_executable(test_socket_utils
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_socket_utils.cpp
    )
    add_dependencies(test_socket_utils 
        lmcomm
    )
    target_include_directories(test_socket_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_socket_utils PUBLIC cxx_std_11)
    target_link_libraries(test_socket_utils lmcomm pthread)
endif()