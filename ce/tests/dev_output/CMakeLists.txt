cmake_minimum_required(VERSION 3.18)
project(dev_output_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(dev_output_tests
    ../../src/dev_output.cc
    dev_output_test.cc
)
add_test(NAME dev_output_tests COMMAND dev_output_tests)
