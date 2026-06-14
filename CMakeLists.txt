cmake_minimum_required(VERSION 4.2.2)

project(
  sivra
  VERSION 0.1
  DESCRIPTION "SIMD Vector Recovery and Analysis"
  LANGUAGES CXX
)

include(FetchContent)
include(CTest)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(SIVRA_BUILD_CLI "Build the SIVRA command-line tool" ${PROJECT_IS_TOP_LEVEL})
option(SIVRA_BUILD_DOCS "Build SIVRA documentation" OFF)
option(
  SIVRA_BUILD_COMPAT_IMPORTERS
  "Build compatibility importers for legacy artifacts"
  ON
)
option(SIVRA_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)
option(SIVRA_ENABLE_COVERAGE "Enable compiler coverage instrumentation" OFF)
set(SIVRA_RUNTIME_OUTPUT_DIRECTORY
    ""
    CACHE PATH
    "Directory where SIVRA executable targets are written."
)
set(SIVRA_LEGACY_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
if(CMAKE_BINARY_DIR MATCHES "/([^/]+)$")
  set(SIVRA_LEGACY_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_MATCH_1}")
endif()
if(NOT SIVRA_RUNTIME_OUTPUT_DIRECTORY OR
   SIVRA_RUNTIME_OUTPUT_DIRECTORY STREQUAL SIVRA_LEGACY_RUNTIME_OUTPUT_DIRECTORY)
  set(SIVRA_RUNTIME_OUTPUT_DIRECTORY
      "${PROJECT_BINARY_DIR}/bin"
      CACHE PATH
      "Directory where SIVRA executable targets are written."
      FORCE
  )
endif()

function(sivra_apply_build_options target)
  if(SIVRA_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${target}
        PRIVATE
          -fno-omit-frame-pointer
          -fsanitize=address,undefined
      )
      target_link_options(${target}
        PRIVATE
          -fsanitize=address,undefined
      )
    else()
      message(FATAL_ERROR "SIVRA_ENABLE_SANITIZERS requires Clang or GCC")
    endif()
  endif()

  if(SIVRA_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${target}
        PRIVATE
          --coverage
      )
      target_link_options(${target}
        PRIVATE
          --coverage
      )
    else()
      message(FATAL_ERROR "SIVRA_ENABLE_COVERAGE requires Clang or GCC")
    endif()
  endif()
endfunction()

function(sivra_configure_executable_output target)
  set_target_properties(${target}
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${SIVRA_RUNTIME_OUTPUT_DIRECTORY}"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${SIVRA_RUNTIME_OUTPUT_DIRECTORY}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${SIVRA_RUNTIME_OUTPUT_DIRECTORY}"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${SIVRA_RUNTIME_OUTPUT_DIRECTORY}"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${SIVRA_RUNTIME_OUTPUT_DIRECTORY}"
  )
endfunction()

if(SIVRA_BUILD_CLI)
  FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.2
    EXCLUDE_FROM_ALL
  )

  FetchContent_MakeAvailable(cli11)
endif()

if(SIVRA_BUILD_COMPAT_IMPORTERS)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    EXCLUDE_FROM_ALL
  )

  FetchContent_MakeAvailable(nlohmann_json)
endif()

if(BUILD_TESTING)
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
    EXCLUDE_FROM_ALL
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

install(
  TARGETS
    sivra-core
    sivra-ir
    sivra-program
    sivra-canonicalizer
    sivra-x86
    sivra-recovery
  EXPORT sivraTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  DIRECTORY
    include/sivra/core
    include/sivra/ir
    include/sivra/program
    include/sivra/canonicalizer
    include/sivra/x86
    include/sivra/recovery
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sivra
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.def"
)

install(
  EXPORT sivraTargets
  FILE sivraTargets.cmake
  NAMESPACE sivra::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sivra
)

configure_package_config_file(
  cmake/sivraConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/sivraConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sivra
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/sivraConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sivraConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sivraConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sivra
)
