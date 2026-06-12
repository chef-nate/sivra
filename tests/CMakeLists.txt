function(add_sivra_test target)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})

  add_executable(${target})

  target_sources(${target}
    PRIVATE
      unit/main.cpp
      ${ARG_SOURCES}
  )

  target_link_libraries(${target}
    PRIVATE
      doctest::doctest
      ${ARG_LIBRARIES}
  )

  target_compile_definitions(${target}
    PRIVATE
      _SILENCE_CXX20_CISO646_REMOVED_WARNING
  )

  target_compile_options(${target}
    PRIVATE
      $<$<CXX_COMPILER_ID:Clang>:-Wno-cpp>
  )

  target_compile_features(${target}
    PRIVATE
      cxx_std_23
  )

  set_target_properties(${target}
    PROPERTIES
      CXX_EXTENSIONS OFF
  )

  sivra_apply_build_options(${target})

  add_test(
    NAME ${target}
    COMMAND ${target}
  )

  if(SIVRA_ENABLE_SANITIZERS)
    set_tests_properties(${target}
      PROPERTIES
        ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
    )
  endif()
endfunction()

add_sivra_test(sivra-core-tests
  SOURCES
    unit/core/core_tests.cpp
  LIBRARIES
    sivra::core
)

add_sivra_test(sivra-ir-tests
  SOURCES
    unit/ir/builtin_operations_tests.cpp
    unit/ir/graph_builder_tests.cpp
    unit/ir/operation_attribute_tests.cpp
    unit/ir/operation_catalogue_tests.cpp
    unit/ir/operation_signature_tests.cpp
    unit/ir/structural_tests.cpp
    unit/ir/value_type_tests.cpp
  LIBRARIES
    sivra::ir
)

add_sivra_test(sivra-canonicalizer-tests
  SOURCES
    unit/canonicalizer/algebraic_rules_tests.cpp
    unit/canonicalizer/configuration_tests.cpp
    unit/canonicalizer/engine_rebuild_tests.cpp
    unit/canonicalizer/evaluator_tests.cpp
    unit/canonicalizer/infrastructure_tests.cpp
    unit/canonicalizer/scheduler_tests.cpp
    unit/canonicalizer/structural_rules_tests.cpp
    unit/canonicalizer/worklist_tests.cpp
    support/expression_format.cpp
  LIBRARIES
    sivra::canonicalizer
)

add_sivra_test(sivra-program-tests
  SOURCES
    unit/program/program_tests.cpp
  LIBRARIES
    sivra::program
)

add_sivra_test(sivra-x86-tests
  SOURCES
    unit/x86/x86_tests.cpp
  LIBRARIES
    sivra::x86
)

set(sivra_unit_test_targets
  sivra-core-tests
  sivra-ir-tests
  sivra-canonicalizer-tests
  sivra-program-tests
  sivra-x86-tests
)

if(SIVRA_BUILD_COMPAT_IMPORTERS)
  add_sivra_test(sivra-compatibility-tests
    SOURCES
      compatibility/raw_expression_json_tests.cpp
    LIBRARIES
      sivra-compat-raw-expression-json
  )
  list(APPEND sivra_unit_test_targets sivra-compatibility-tests)
endif()

add_custom_target(sivra-unittests
  DEPENDS ${sivra_unit_test_targets}
)

if(SIVRA_BUILD_CLI)
  add_test(
    NAME sivra-cli-smoke
    COMMAND sivra dot-product
  )
  if(SIVRA_ENABLE_SANITIZERS)
    set_tests_properties(sivra-cli-smoke
      PROPERTIES
        ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
    )
  endif()
endif()

add_test(
  NAME sivra-package-smoke
  COMMAND
    ${CMAKE_COMMAND}
    -DSIVRA_BINARY_DIR=${PROJECT_BINARY_DIR}
    -DSIVRA_SMOKE_SOURCE_DIR=${PROJECT_SOURCE_DIR}/cmake/package-smoke
    -DSIVRA_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DSIVRA_GENERATOR=${CMAKE_GENERATOR}
    -DSIVRA_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DSIVRA_ENABLE_SANITIZERS=${SIVRA_ENABLE_SANITIZERS}
    -P ${PROJECT_SOURCE_DIR}/cmake/run-package-smoke.cmake
)
