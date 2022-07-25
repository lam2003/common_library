project(lmcomm)
cmake_minimum_required(VERSION 3.16)

if(BUILD_LMCOMM_WITH_DEBUG)
    message("####################### build width debug #######################")
    add_compile_options(-g3 -O0)
else()
    message("####################### build width release #######################")
    add_compile_options(-O3)
endif()

add_library(lmcomm
    ${CMAKE_CURRENT_SOURCE_DIR}/net/socket_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/net/socket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/net/dns_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/net/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/poller/event_poller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/poller/pipe_wrapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/poller/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/logger.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/uv_error.cpp
)

target_include_directories(lmcomm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(lmcomm PUBLIC cxx_std_11)
target_compile_options(lmcomm PUBLIC -Wall)


if(BUILD_LMCOMM_TEST)
    message("####################### build test #######################")

    add_executable(test_listen
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_listen.cpp
    )
    add_dependencies(test_listen 
        lmcomm
    )
    target_include_directories(test_listen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_listen PUBLIC cxx_std_11)
    target_link_libraries(test_listen lmcomm pthread)

    add_executable(test_connect
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_connect.cpp
    )
    add_dependencies(test_connect 
        lmcomm
    )
    target_include_directories(test_connect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_connect PUBLIC cxx_std_11)
    target_link_libraries(test_connect lmcomm pthread)

    add_executable(test_udp_listen
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_udp_listen.cpp
    )
    add_dependencies(test_udp_listen 
        lmcomm
    )
    target_include_directories(test_udp_listen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_udp_listen PUBLIC cxx_std_11)
    target_link_libraries(test_udp_listen lmcomm pthread)
endif()

# set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/sync/arch_ctx.S PROPERTY LANGUAGE C)
# add_executable(testctx
#     ${CMAKE_CURRENT_SOURCE_DIR}/sync/arch_ctx.S
#     ${CMAKE_CURRENT_SOURCE_DIR}/sync/main.cpp
# )