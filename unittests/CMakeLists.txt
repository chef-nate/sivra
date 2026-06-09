add_executable(sivra-unittests)

target_sources(sivra-unittests
  PRIVATE
    ir/expression_graph_type_tests.cpp
    ir/type_context_tests.cpp
    main.cpp
)

target_link_libraries(sivra-unittests
  PRIVATE
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
