cmake_minimum_required(VERSION 3.18)
project(map_clip_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(map_clip_tests
    ../../src/scene_renderer/map_clip.cc
    map_clip_test.cc
)
target_include_directories(map_clip_tests PRIVATE ../../src/scene_renderer)
add_test(NAME map_clip_tests COMMAND map_clip_tests)
