add_library(sivra-core STATIC)
add_library(sivra::core ALIAS sivra-core)

target_sources(sivra-core
  PRIVATE
    core/budget.cpp
    core/diagnostic.cpp
    core/owner_token.cpp
    core/source_location.cpp
    core/trace.cpp
)

target_include_directories(sivra-core
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sivra-core
  PUBLIC
    cxx_std_23
)

set_target_properties(sivra-core
  PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME core
)

sivra_apply_build_options(sivra-core)

add_library(sivra-ir STATIC)
add_library(sivra::ir ALIAS sivra-ir)

target_sources(sivra-ir
  PRIVATE
    ir/builtins/operations.cpp
    ir/constant.cpp
    ir/expression_graph.cpp
    ir/expression_node.cpp
    ir/graph_builder.cpp
    ir/id.cpp
    ir/operation.cpp
    ir/operation_attribute.cpp
    ir/operation_catalogue.cpp
    ir/operation_signature.cpp
    ir/structural.cpp
    ir/validation.cpp
    ir/value_type.cpp
)

target_include_directories(sivra-ir
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sivra-ir
  PUBLIC
    cxx_std_23
)

target_link_libraries(sivra-ir
  PUBLIC
    sivra::core
)

set_target_properties(sivra-ir
  PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME ir
)

sivra_apply_build_options(sivra-ir)

add_library(sivra-program STATIC)
add_library(sivra::program ALIAS sivra-program)

target_sources(sivra-program
  PRIVATE
    program/decoded_program.cpp
    program/id.cpp
    program/instruction_form.cpp
    program/operand.cpp
)

target_include_directories(sivra-program
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sivra-program
  PUBLIC
    cxx_std_23
)

set_target_properties(sivra-program
  PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME program
)

target_link_libraries(sivra-program
  PUBLIC
    sivra::core
    sivra::ir
)

sivra_apply_build_options(sivra-program)

add_library(sivra-canonicalizer STATIC)
add_library(sivra::canonicalizer ALIAS sivra-canonicalizer)

target_sources(sivra-canonicalizer
  PRIVATE
    canonicalizer/configuration.cpp
    canonicalizer/engine.cpp
    canonicalizer/equivalence_contract.cpp
    canonicalizer/evaluator_catalogue.cpp
    canonicalizer/result.cpp
    canonicalizer/rewrite_context.cpp
    canonicalizer/rule.cpp
    canonicalizer/rule_catalogue.cpp
    canonicalizer/scheduler.cpp
    canonicalizer/termination.cpp
    canonicalizer/trace.cpp
    canonicalizer/worklist.cpp
    canonicalizer/rules/algebraic.cpp
    canonicalizer/rules/constants.cpp
    canonicalizer/rules/structural.cpp
)

target_include_directories(sivra-canonicalizer
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sivra-canonicalizer
  PUBLIC
    cxx_std_23
)

set_target_properties(sivra-canonicalizer
  PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME canonicalizer
)

target_link_libraries(sivra-canonicalizer
  PUBLIC
    sivra::core
    sivra::ir
)

sivra_apply_build_options(sivra-canonicalizer)

add_library(sivra-x86 STATIC)
add_library(sivra::x86 ALIAS sivra-x86)

target_sources(sivra-x86
  PRIVATE
    x86/form_resolver.cpp
    x86/instruction_catalogue.cpp
    x86/parser.cpp
    x86/register_catalogue.cpp
    x86/semantic_provider.cpp
    x86/tokenizer.cpp
)

target_include_directories(sivra-x86
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sivra-x86
  PUBLIC
    cxx_std_23
)

set_target_properties(sivra-x86
  PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME x86
)

target_link_libraries(sivra-x86
  PUBLIC
    sivra::program
)

sivra_apply_build_options(sivra-x86)

if(SIVRA_BUILD_COMPAT_IMPORTERS)
  add_library(sivra-compat-raw-expression-json STATIC)

  target_sources(sivra-compat-raw-expression-json
    PRIVATE
      compat/raw_expression_json/raw_expression_json.cpp
  )

  target_include_directories(sivra-compat-raw-expression-json
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/compat/raw_expression_json
  )

  target_compile_features(sivra-compat-raw-expression-json
    PUBLIC
      cxx_std_23
  )

  set_target_properties(sivra-compat-raw-expression-json
    PROPERTIES
      CXX_EXTENSIONS OFF
  )

  target_link_libraries(sivra-compat-raw-expression-json
    PUBLIC
      sivra::ir
    PRIVATE
      nlohmann_json::nlohmann_json
  )

  sivra_apply_build_options(sivra-compat-raw-expression-json)
endif()
