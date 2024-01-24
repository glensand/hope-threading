cmake_minimum_required(VERSION 3.11)

project(hope-thread-main)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(test)
add_subdirectory(samples/sync)
add_subdirectory(samples/hash_map_perf_test)
add_subdirectory(lib)