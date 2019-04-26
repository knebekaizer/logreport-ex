cmake_minimum_required(VERSION 3.14)
project(iplog_test)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror" )

include_directories( . )

set(SOURCE_FILES
        main.cpp
        subnets.cpp
        trie.cpp
    #    net/radix.c
    #    cust_db.cpp cust_db.h report.cpp report.h subnets.cpp subnets.h
)

add_executable(iplog ${SOURCE_FILES} )
add_executable(iplog_DEBUG ${SOURCE_FILES} )

add_executable(iplog_ut ${SOURCE_FILES})

target_compile_definitions(iplog_ut PRIVATE
        UT_CATCH
)

target_compile_definitions(iplog_DEBUG PRIVATE
        IPLOG_DEBUG
)
