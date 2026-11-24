cmake_minimum_required(VERSION 3.20)
project(AcousticFX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)           # plain -std=c++20, not gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # helps VS Code / clangd find headers

# Default to Debug if no build type was given at configure time
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Effects library: pure DSP, no file or audio-device code
add_library(dsp STATIC
    dsp/gain.cpp
    dsp/tremolo.cpp
    dsp/delay.cpp
    dsp/chorus.cpp
)
target_include_directories(dsp PUBLIC dsp)
target_compile_options(dsp PRIVATE -Wall -Wextra)

# WAV parser: file input/output, kept out of dsp
add_library(io STATIC
    io/wav.cpp
)
target_include_directories(io PUBLIC io)
target_compile_options(io PRIVATE -Wall -Wextra)

# WAV in -> WAV out tool
add_executable(offline_fx
    apps/offline_fx/main.cpp
)
target_link_libraries(offline_fx PRIVATE dsp io)
target_compile_options(offline_fx PRIVATE -Wall -Wextra)
