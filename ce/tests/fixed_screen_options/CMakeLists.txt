cmake_minimum_required(VERSION 3.18)
project(fixed_screen_options_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(fixed_screen_options_tests
    ../../src/devtools/fixed_screen_options.cc
    fixed_screen_options_test.cc
)
add_test(NAME fixed_screen_options_tests COMMAND fixed_screen_options_tests)
