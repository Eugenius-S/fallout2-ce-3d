cmake_minimum_required(VERSION 3.18)
project(sun_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(sun_tests
    ../../src/scene/sun.cc
    sun_test.cc
)
target_include_directories(sun_tests PRIVATE ../../src/scene)
add_test(NAME sun_tests COMMAND sun_tests)
