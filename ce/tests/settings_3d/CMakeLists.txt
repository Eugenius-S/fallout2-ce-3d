cmake_minimum_required(VERSION 3.18)
project(settings_3d_tests LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

include(FetchContent)
FetchContent_Declare(sdl2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-2.32.8
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(sdl2)
if(NOT sdl2_POPULATED)
    FetchContent_Populate(sdl2)
endif()
if(NOT TARGET SDL2-static)
    add_subdirectory(${sdl2_SOURCE_DIR} ${sdl2_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib
    GIT_TAG v1.3.1
    GIT_SHALLOW TRUE
)
set(ZLIB_BUILD_EXAMPLES OFF)
FetchContent_GetProperties(zlib)
if(NOT zlib_POPULATED)
    FetchContent_Populate(zlib)
endif()
if(NOT TARGET zlibstatic)
    add_subdirectory(${zlib_SOURCE_DIR} ${zlib_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(settings_3d_tests
    ../../src/settings.cc
    ../../src/scene/scene_settings.cc
    ../../src/scene/menu_text.cc
    settings_3d_test.cc
)
target_include_directories(settings_3d_tests PRIVATE
    ../../src
    ${sdl2_SOURCE_DIR}/include
    ${sdl2_BINARY_DIR}/include
    ${zlib_SOURCE_DIR}
    ${zlib_BINARY_DIR}
)
target_compile_definitions(settings_3d_tests PRIVATE SDL_MAIN_HANDLED)
add_test(NAME settings_3d_tests COMMAND settings_3d_tests)

add_executable(options_panel_model_tests
    ../../src/options_panel_model.cc
    ../../src/options_schema.cc
    options_panel_model_test.cc
)
target_include_directories(options_panel_model_tests PRIVATE
    ../../src
    ${sdl2_SOURCE_DIR}/include
    ${sdl2_BINARY_DIR}/include
    ${zlib_SOURCE_DIR}
    ${zlib_BINARY_DIR}
)
target_compile_definitions(options_panel_model_tests PRIVATE SDL_MAIN_HANDLED)
add_test(NAME options_panel_model_tests COMMAND options_panel_model_tests)
