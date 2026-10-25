cmake_minimum_required(VERSION 3.18)
project(embedded_fonts_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(embedded_fonts_tests embedded_fonts_test.cc
    ../../src/scene/text_layer.cc
    ../../src/scene_renderer/text_draw.cc
)
target_include_directories(embedded_fonts_tests PRIVATE ../../src)
include(../../cmake/EmbeddedFonts.cmake)
fallout_embed_fonts(embedded_fonts_tests)
add_test(NAME embedded_fonts_tests COMMAND embedded_fonts_tests)
