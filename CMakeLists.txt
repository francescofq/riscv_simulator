cmake_minimum_required(VERSION 3.16)
project(riscv_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(riscv-sim
    src/main.cpp
    src/cli.cpp
    src/cpu.cpp
    src/decoder.cpp
    src/memory.cpp
)

target_include_directories(riscv-sim PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(riscv-sim PRIVATE -Wall -Wextra -Wpedantic)
endif()
