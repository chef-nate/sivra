add_library(sivra-core STATIC)
add_library(sivra::core ALIAS sivra-core)

target_sources(sivra-core
  PRIVATE
    core/budget.cpp
    core/owner_token.cpp
)

target_include_directories(sivra-core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
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
)

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
    ir/operation_catalogue.cpp
    ir/value_type.cpp
)

target_include_directories(sivra-ir
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
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
)

add_library(sivra-canonicalizer STATIC)
add_library(sivra::canonicalizer ALIAS sivra-canonicalizer)

target_sources(sivra-canonicalizer
  PRIVATE
    canonicalizer/engine.cpp
    canonicalizer/options.cpp
    canonicalizer/rules.cpp
)

target_include_directories(sivra-canonicalizer
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
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
)

target_link_libraries(sivra-canonicalizer
  PUBLIC
    sivra::core
    sivra::ir
)

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
endif()
