cmake_minimum_required(VERSION 3.10)
project(iplog_test)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror" )

include_directories( . )

set(SOURCE_FILES
        main.cpp
        subnets.cpp
        trie.cpp
        IpSummary.cpp IpSummary.h)

add_executable(iplog ${SOURCE_FILES} IpSummary.cpp IpSummary.h)
add_executable(iplog_SELFTEST ${SOURCE_FILES} IpSummary.cpp IpSummary.h)

add_executable(iplog_ut ${SOURCE_FILES} IpSummary.cpp IpSummary.h)

target_compile_definitions(iplog PRIVATE
        DEF_LOG_LEVEL=LOG_LEVEL::info
        NDEBUG
)

target_compile_definitions(iplog_ut PRIVATE
        IPLOG_SELFTEST
        UT_CATCH
)

target_compile_definitions(iplog_SELFTEST PRIVATE
        IPLOG_SELFTEST
)
