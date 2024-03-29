cmake_minimum_required(VERSION 3.0)
project(sy)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/yaml-cpp)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/jsoncpp-1.8.4)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/sqlite-3.28.0)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/tinyxml2-7.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(/apps/sy/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/yaml-cpp/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/jsoncpp-1.8.4/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/sqlite-3.28.0)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/tinyxml2-7.0.1)
link_directories(/apps/sy/lib)
link_directories(/apps/sy/lib64)

option(BUILD_TEST "ON for complile test" OFF)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(MySQL REQUIRED)
if(MYSQL_FOUND)
    include_directories(${MYSQL_INCLUDE_DIR})
endif()

find_package(Protobuf)
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
    set(Protobuf_LIBRARY "/usr/lib64/libprotoc.so.8")
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIR})
endif()

set(LIB_SRC
    sy/address.cc
    sy/bytearray.cc
    sy/config.cc
    sy/db/fox_thread.cc
    sy/db/mysql.cc
    sy/db/redis.cc
    sy/db/sqlite3.cc
    sy/ds/bitmap.cc
    sy/ds/roaring_bitmap.cc
    sy/ds/roaring.c
    sy/ds/util.cc
    sy/email/email.cc
    sy/email/smtp.cc
    sy/env.cc
    sy/daemon.cc
    sy/fd_manager.cc
    sy/fiber.cc
    sy/http/http.cc
    sy/http/http_connection.cc
    sy/http/http_parser.cc
    sy/http/http_session.cc
    sy/http/http_server.cc
    sy/http/servlet.cc
    sy/http/servlets/config_servlet.cc
    sy/http/servlets/status_servlet.cc
    sy/http/session_data.cc
    sy/http/ws_connection.cc
    sy/http/ws_session.cc
    sy/http/ws_server.cc
    sy/http/ws_servlet.cc
    sy/hook.cc
    sy/iomanager.cc
    sy/library.cc
    sy/log.cc
    sy/module.cc
    sy/mutex.cc
    sy/ns/name_server_module.cc
    sy/ns/ns_client.cc
    sy/ns/ns_protocol.cc
    sy/protocol.cc
    sy/rock/rock_protocol.cc
    sy/rock/rock_server.cc
    sy/rock/rock_stream.cc
    sy/scheduler.cc
    sy/socket.cc
    sy/stream.cc
    sy/streams/async_socket_stream.cc
    sy/streams/socket_stream.cc
    sy/streams/load_balance.cc
    sy/streams/service_discovery.cc
    sy/streams/zlib_stream.cc
    sy/tcp_server.cc
    sy/timer.cc
    sy/thread.cc
    sy/util.cc
    sy/util/crypto_util.cc
    sy/util/json_util.cc
    sy/util/hash_util.cc
    sy/worker.cc
    sy/application.cc
    sy/zk_client.cc
    )

ragelmaker(sy/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sy/http)
ragelmaker(sy/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sy/http)
ragelmaker(sy/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sy)

protobufmaker(sy/ns/ns_protobuf.proto LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR})
#PROTOBUF_GENERATE_CPP(PB_SRCS PB_HDRS sy/ns/ns_protobuf.proto)
#message("****${PB_SRCS}***${PB_HDRS}****")
#list(APPEND LIB_SRC ${PB_SRCS})
#
#message(STATUS ${LIB_SRC})

add_library(sy SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sy)
#add_library(sy_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (sy_static PROPERTIES OUTPUT_NAME "sy")

#find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)
#set(LIBS ${LIBS} sy)
#set(LIBS ${LIBS} dl)
#set(LIBS ${LIBS} ${YAMLCPP})
#set(LIBS ${LIBS} ${PTHREAD})

set(LIBS
        sy
        dl
        pthread
        yaml-cpp
        jsoncpp
        ${ZLIB_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        event
        hiredis_vip
        mysqlclient_r
        zookeeper_mt
        sqlite3
        tinyxml2
        jemalloc
        )
sy_add_executable(test_util "tests/test_util.cc" sy "${LIBS}")
sy_add_executable(test_hashmultimap "tests/test_hashmultimap.cc" sy "${LIBS}")
sy_add_executable(test_hashmap "tests/test_hashmap.cc" sy "${LIBS}")
sy_add_executable(test_dict "tests/test_dict.cc" sy "${LIBS}")
sy_add_executable(test_array "tests/test_array.cc" sy "${LIBS}")
if(BUILD_TEST)
sy_add_executable(test1 "tests/test.cc" sy "${LIBS}")
sy_add_executable(test_config "tests/test_config.cc" sy "${LIBS}")
sy_add_executable(test_thread "tests/test_thread.cc" sy "${LIBS}")
sy_add_executable(test_fiber "tests/test_fiber.cc" sy "${LIBS}")
sy_add_executable(test_scheduler "tests/test_scheduler.cc" sy "${LIBS}")
sy_add_executable(test_iomanager "tests/test_iomanager.cc" sy "${LIBS}")
sy_add_executable(test_hook "tests/test_hook.cc" sy "${LIBS}")
sy_add_executable(test_address "tests/test_address.cc" sy "${LIBS}")
sy_add_executable(test_socket "tests/test_socket.cc" sy "${LIBS}")
sy_add_executable(test_bytearray "tests/test_bytearray.cc" sy "${LIBS}")
sy_add_executable(test_http "tests/test_http.cc" sy "${LIBS}")
sy_add_executable(test_http_parser "tests/test_http_parser.cc" sy "${LIBS}")
sy_add_executable(test_tcp_server "tests/test_tcp_server.cc" sy "${LIBS}")
sy_add_executable(echo_server "examples/echo_server.cc" sy "${LIBS}")
sy_add_executable(test_http_server "tests/test_http_server.cc" sy "${LIBS}")
sy_add_executable(test_uri "tests/test_uri.cc" sy "${LIBS}")
sy_add_executable(my_http_server "samples/my_http_server.cc" sy "${LIBS}")

sy_add_executable(echo_server_udp "examples/echo_server_udp.cc" sy "${LIBS}")
sy_add_executable(echo_udp_client "examples/echo_udp_client.cc" sy "${LIBS}")

sy_add_executable(test_daemon "tests/test_daemon.cc" sy "${LIBS}")
sy_add_executable(test_env "tests/test_env.cc" sy "${LIBS}")
sy_add_executable(test_ws_server "tests/test_ws_server.cc" sy "${LIBS}")
sy_add_executable(test_ws_client "tests/test_ws_client.cc" sy "${LIBS}")
sy_add_executable(test_application "tests/test_application.cc" sy "${LIBS}")

sy_add_executable(test_http_connection "tests/test_http_connection.cc" sy "${LIBS}")
sy_add_executable(test_lru "tests/test_lru.cc" sy "${LIBS}")
sy_add_executable(test_timed_cache "tests/test_timed_cache.cc" sy "${LIBS}")
sy_add_executable(test_timed_lru_cache "tests/test_timed_lru_cache.cc" sy "${LIBS}")
sy_add_executable(test_zlib_stream "tests/test_zlib_stream.cc" sy "${LIBS}")

endif()
sy_add_executable(test_crypto "tests/test_crypto.cc" sy "${LIBS}")
sy_add_executable(test_sqlite3 "tests/test_sqlite3.cc" sy "${LIBS}")
sy_add_executable(test_rock "tests/test_rock.cc" sy "${LIBS}")
sy_add_executable(test_email  "tests/test_email.cc" sy "${LIBS}")
sy_add_executable(test_mysql "tests/test_mysql.cc" sy "${LIBS}")
sy_add_executable(test_nameserver "tests/test_nameserver.cc" sy "${LIBS}")
sy_add_executable(test_bitmap "tests/test_bitmap.cc" sy "${LIBS}")
sy_add_executable(test_zkclient "tests/test_zookeeper.cc" sy "${LIBS}")
sy_add_executable(test_service_discovery "tests/test_service_discovery.cc" sy "${LIBS}")

set(ORM_SRCS
    sy/orm/table.cc
    sy/orm/column.cc
    sy/orm/index.cc
    sy/orm/orm.cc
    sy/orm/util.cc
    )
sy_add_executable(orm "${ORM_SRCS}" sy "${LIBS}")

add_library(test_module SHARED tests/test_module.cc)

sy_add_executable(bin_sy "sy/main.cc" sy "${LIBS}")
set_target_properties(bin_sy PROPERTIES OUTPUT_NAME "sy")

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/orm_out)
#set(OLIBS ${LIBS} orm_data)
#sy_add_executable(test_orm "tests/test_orm.cc" orm_data "${OLIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
