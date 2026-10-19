cmake_minimum_required(VERSION 3.18)
project(snapshot_rows_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(snapshot_rows_tests
    ../../src/devtools/snapshot_rows.cc
    snapshot_rows_test.cc
)
add_test(NAME snapshot_rows_tests COMMAND snapshot_rows_tests)
