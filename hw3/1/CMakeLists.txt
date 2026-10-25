cmake_minimum_required(VERSION 3.14)
project(nth_power_test CXX)
 
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
 
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.18.0)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_executable(${PROJECT_NAME} tst_nth_power.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main GTest::gmock)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
 
