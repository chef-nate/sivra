add_executable(sivra-unittests)

target_sources(sivra-unittests
  PRIVATE
    unit/core/core_tests.cpp
    unit/canonicalizer/algebraic_rules_tests.cpp
    unit/canonicalizer/engine_rebuild_tests.cpp
    unit/canonicalizer/options_tests.cpp
    unit/ir/builtin_operations_tests.cpp
    unit/ir/graph_builder_tests.cpp
    unit/ir/operation_catalogue_tests.cpp
    unit/ir/value_type_tests.cpp
    unit/main.cpp
    support/expression_format.cpp
)

if(SIVRA_BUILD_COMPAT_IMPORTERS)
  target_sources(sivra-unittests
    PRIVATE
      compatibility/raw_expression_json_tests.cpp
  )
endif()

target_link_libraries(sivra-unittests
  PRIVATE
    sivra::canonicalizer
    sivra::ir
    doctest::doctest
)

if(SIVRA_BUILD_COMPAT_IMPORTERS)
  target_link_libraries(sivra-unittests
    PRIVATE
      sivra-compat-raw-expression-json
  )
endif()

target_compile_definitions(sivra-unittests
  PRIVATE
    _SILENCE_CXX20_CISO646_REMOVED_WARNING
)

target_compile_options(sivra-unittests
  PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wno-cpp>
)

target_compile_features(sivra-unittests
  PRIVATE
    cxx_std_23
)

set_target_properties(sivra-unittests
  PROPERTIES
    CXX_EXTENSIONS OFF
)

add_test(
  NAME sivra-unittests
  COMMAND sivra-unittests
)

if(SIVRA_BUILD_CLI)
  add_test(
    NAME sivra-cli-smoke
    COMMAND sivra dot-product
  )
endif()
