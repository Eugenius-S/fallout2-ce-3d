cmake_minimum_required(VERSION 3.24)
project(fallout2_native_dlss5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# No C++20 named modules in this codebase; the Ninja+MSVC module scanner is
# also mis-generated on CMake 4.4.4 with an explicit build type, so disable it.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(NOT WIN32)
    message(FATAL_ERROR "This prototype targets Windows/D3D12 only")
endif()

add_executable(f2native
    src/main.cpp
    src/app.cpp
    src/gpu.cpp
    src/render.cpp
    src/neural.cpp
    src/camera.cpp
)
target_include_directories(f2native PRIVATE ${CMAKE_SOURCE_DIR}/vendor/ngx/include)
target_link_libraries(f2native PRIVATE
    d3d12 dxgi dxguid d3dcompiler user32 gdi32 dbghelp
    ${CMAKE_SOURCE_DIR}/vendor/ngx/lib/Windows_x86_64/x64/nvsdk_ngx_d.lib)
target_compile_definitions(f2native PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
# Line-level crash forensics even in Release.
target_compile_options(f2native PRIVATE /Zi)
target_link_options(f2native PRIVATE /DEBUG:FULL)

enable_testing()
add_executable(f2native_tests tests/contracts.cpp)
add_test(NAME contracts COMMAND f2native_tests)
