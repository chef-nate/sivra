cmake_minimum_required(VERSION 4.2.2)

project(
  sivra
  VERSION 0.1
  DESCRIPTION "SIMD Vector Recovery and Analysis"
  LANGUAGES CXX
)

include(FetchContent)
include(CTest)

option(SIVRA_BUILD_CLI "Build the SIVRA command-line tool" ${PROJECT_IS_TOP_LEVEL})
option(
  SIVRA_BUILD_COMPAT_IMPORTERS
  "Build compatibility importers for legacy artifacts"
  ON
)

if(SIVRA_BUILD_CLI)
  FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.2
  )

  FetchContent_MakeAvailable(cli11)
endif()

if(SIVRA_BUILD_COMPAT_IMPORTERS)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
  )

  FetchContent_MakeAvailable(nlohmann_json)
endif()

if(BUILD_TESTING)
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
  )

  FetchContent_MakeAvailable(doctest)
endif()

add_subdirectory(lib)

if(SIVRA_BUILD_CLI)
  if(NOT SIVRA_BUILD_COMPAT_IMPORTERS)
    message(FATAL_ERROR "SIVRA_BUILD_CLI currently requires SIVRA_BUILD_COMPAT_IMPORTERS")
  endif()

  add_subdirectory(tools/sivra)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
