cmake_minimum_required(VERSION 3.15)

set(REV 1.0.10)

add_definitions(-DBUILD_REVISION="${REV}")

project(zspec VERSION ${REV} LANGUAGES CXX)

#set(CMAKE_CXX_COMPILER "/usr/local/bin/g++-14")
#enable_testing()

set(EXE_NAME zspec)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS off)
set(CMAKE_CXX_FLAGS "-g3 -O3")

message (${CMAKE_CXX_COMPILER})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_compile_definitions(TOML_EXCEPTIONS=1)

set(SOURCE_FILES
    src/core/test_spec_executor.cpp
    src/core/parser.cpp
    src/main.cpp
)
set(INCLUDES "/usr/local/Cellar/gcc/14.1.0/include")

add_executable(${EXE_NAME} ${SOURCE_FILES})