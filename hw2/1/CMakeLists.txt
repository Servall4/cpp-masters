cmake_minimum_required(VERSION 3.14)
project(cow_string_bench CXX)
 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
 
find_package(benchmark QUIET)
if(NOT benchmark_FOUND)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1)
    FetchContent_MakeAvailable(benchmark)
endif()
 
add_executable(cow-bench cow_bench.cpp)
target_link_libraries(cow-bench PRIVATE benchmark::benchmark_main)
 
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.18.0)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_executable(${PROJECT_NAME} tst_cow_string.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
 
