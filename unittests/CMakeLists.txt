add_executable(sivra-unittests)

target_sources(sivra-unittests
  PRIVATE
    canonicalizer/algebraic_rules_tests.cpp
    canonicalizer/associative_flattening_tests.cpp
    canonicalizer/engine_context_tests.cpp
    canonicalizer/engine_rebuild_tests.cpp
    canonicalizer/options_tests.cpp
    canonicalizer/rule_pipeline_tests.cpp
    canonicalizer/typed_algebraic_rules_tests.cpp
    ir/builtin_operations_tests.cpp
    ir/expression_graph_child_tests.cpp
    ir/expression_graph_context_tests.cpp
    ir/expression_graph_type_tests.cpp
    ir/ir_context_tests.cpp
    ir/operation_registry_tests.cpp
    ir/type_context_tests.cpp
    main.cpp
    support/expression_format.cpp
)

target_link_libraries(sivra-unittests
  PRIVATE
    sivra::canonicalizer
    sivra::ir
    doctest::doctest
)

target_compile_definitions(sivra-unittests
  PRIVATE
    _SILENCE_CXX20_CISO646_REMOVED_WARNING
)

target_compile_options(sivra-unittests
  PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wno-cpp>
)

add_test(
  NAME sivra-unittests
  COMMAND sivra-unittests
)
