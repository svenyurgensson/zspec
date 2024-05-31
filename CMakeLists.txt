cmake_minimum_required(VERSION 3.15)

project(zspec VERSION 0.0.1 LANGUAGES CXX)

#set(CMAKE_CXX_COMPILER "/usr/local/bin/g++-14")

set(EXE_NAME zspec)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS off)

message (${CMAKE_CXX_COMPILER})

add_compile_definitions(TOML_EXCEPTIONS=1)

set(SOURCE_FILES
    src/core/test_spec_executor.cpp  
    src/core/parser.cpp
    src/main.cpp  
)
set(INCLUDES "/usr/local/Cellar/gcc/14.1.0/include")

add_executable(${EXE_NAME} ${SOURCE_FILES})