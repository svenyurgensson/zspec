cmake_minimum_required(VERSION 3.15)

set(
REV 1.0.11
)

add_definitions(-DBUILD_REVISION="${REV}")

project(zspec VERSION ${REV} LANGUAGES CXX)

#set(CMAKE_CXX_COMPILER "/usr/local/bin/g++-14")
#enable_testing()

set(EXE_NAME zspec)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS off)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-g3 -O3 -ferror-limit=1 -ftemplate-backtrace-limit=1")
else()
    set(CMAKE_CXX_FLAGS "-g3 -O3")
endif()

message (${CMAKE_CXX_COMPILER})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_compile_definitions(TOML_EXCEPTIONS=1)
add_compile_definitions(TOML_ENABLE_WINDOWS_COMPAT=1)

set(SOURCE_FILES
    src/core/test_spec_executor.cpp
    src/core/parser.cpp
    src/main.cpp
)
set(INCLUDES "/usr/local/Cellar/gcc/14.1.0/include")

add_executable(${EXE_NAME} ${SOURCE_FILES})