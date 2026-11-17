cmake_minimum_required(VERSION 3.20)
project(AcousticFX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # helps VS Code / clangd find headers

# Your effects library
add_library(dsp STATIC
    dsp/gain.cpp        # needs at least one .cpp; an empty file is fine to start
)
target_include_directories(dsp PUBLIC dsp)
target_compile_options(dsp PRIVATE -Wall -Wextra)

# WAV in -> WAV out tool
add_executable(offline_fx apps/offline_fx/main.cpp)
target_link_libraries(offline_fx PRIVATE dsp)
target_include_directories(offline_fx PRIVATE third_party)