cmake_minimum_required(VERSION 3.11)

project(jerkthread)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

file(GLOB CL_HEADERS
    jerk-thread/*.h
    jerk-thread/platform/*.h
    jerk-thread/containers/queue/*.h
    jerk-thread/containers/hashmap/*.h
    jerk-thread/synchronization/*.h
    jerk-thread/runtime/*.h
)

add_library(jerkthread INTERFACE ${CL_HEADERS})
target_include_directories(jerkthread INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_INSTALL_DIRECTORY ${CMAKE_BINARY_DIR})

INSTALL(DIRECTORY jerkthread/ DESTINATION ${CMAKE_INSTALL_DIRECTORY}/include/jerkthread
        FILES_MATCHING PATTERN "*.h")
