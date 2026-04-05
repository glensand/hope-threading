cmake_minimum_required(VERSION 3.11)

project(spmc_bounded_non_uniform_queue_sample)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(spmc_bounded_non_uniform_queue_sample main.cpp)

target_include_directories(spmc_bounded_non_uniform_queue_sample PUBLIC ../../lib)
add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE=1)
