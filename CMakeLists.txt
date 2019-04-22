cmake_minimum_required(VERSION 3.14)
project(iplog_test)

set(CMAKE_CXX_STANDARD 14)

include_directories( . )

set(SOURCE_FILES
        main.cpp
        net/radix.c
        cust_db.cpp cust_db.h report.cpp report.h)

add_executable(iplog_test ${SOURCE_FILES})