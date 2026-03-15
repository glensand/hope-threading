cmake_minimum_required(VERSION 3.11)

project(hope-thread-test)
set(BINARY hope-thread-test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("../lib")
add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE=1)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB TEST_HEADERS
    *.h
    )
    
file(GLOB TEST_SOURSES
    *.cpp
    )

add_executable(${BINARY} ${TEST_HEADERS} ${TEST_SOURSES})
add_test(NAME ${BINARY} COMMAND ${BINARY})