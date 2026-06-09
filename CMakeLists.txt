cmake_minimum_required(VERSION 4.2.2)

project(
  sivra
  VERSION 0.1
  DESCRIPTION "SIMD Vector Recovery and Analysis"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include(FetchContent)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.6.2
)

FetchContent_MakeAvailable(cli11)

FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.12
)

FetchContent_MakeAvailable(doctest)

enable_testing()

add_subdirectory(lib)
add_subdirectory(tools/sivra)
add_subdirectory(unittests)
