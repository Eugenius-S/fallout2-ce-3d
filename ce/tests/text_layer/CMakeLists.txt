cmake_minimum_required(VERSION 3.18)
project(text_layer_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(text_layer_tests
    ../../src/scene/text_layer.cc
    text_layer_test.cc
)
target_include_directories(text_layer_tests PRIVATE ../../src/scene)
add_test(NAME text_layer_tests COMMAND text_layer_tests)

add_executable(text_capture_tests
    ../../src/scene/text_layer.cc
    ../../src/scene/floating_text.cc
    ../../src/scene/text_capture.cc
    text_capture_test.cc
)
target_include_directories(text_capture_tests PRIVATE ../../src ../../src/scene)
add_test(NAME text_capture_tests COMMAND text_capture_tests)
