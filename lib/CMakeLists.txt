cmake_minimum_required(VERSION 3.11)

project(hope_thread)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

file(GLOB CL_HEADERS
    hope_thread/*.h
    hope_thread/platform/*.h
    hope_thread/containers/queue/*.h
    hope_thread/containers/hashmap/*.h
    hope_thread/synchronization/*.h
    hope_thread/runtime/*.h
)

add_library(hope_thread INTERFACE ${CL_HEADERS})
target_include_directories(hope_thread INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_INSTALL_DIRECTORY ${CMAKE_BINARY_DIR})

INSTALL(DIRECTORY hope_thread/ DESTINATION ${CMAKE_INSTALL_DIRECTORY}/include/hope_thread
        FILES_MATCHING PATTERN "*.h")
