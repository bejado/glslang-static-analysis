cmake_minimum_required(VERSION 3.11)
project(GLSLASTPrinter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENABLE_OPT OFF)
include(FetchContent)
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG main
)

FetchContent_MakeAvailable(glslang)

add_executable(glsl_ast_printer src/main.cpp)

target_link_libraries(glsl_ast_printer PRIVATE glslang)
