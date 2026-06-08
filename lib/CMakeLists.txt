add_library(sivra-ir STATIC)
add_library(sivra::ir ALIAS sivra-ir)

target_sources(sivra-ir
  PRIVATE
    ir/expression_graph.cpp
    ir/expression_node.cpp
    ir/id.cpp
    ir/operation.cpp
    ir/operation_registry.cpp
    ir/type.cpp
)

target_include_directories(sivra-ir
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(sivra-canonicalizer STATIC)
add_library(sivra::canonicalizer ALIAS sivra-canonicalizer)

target_sources(sivra-canonicalizer
  PRIVATE
    canonicalizer/engine.cpp
    canonicalizer/options.cpp
)

target_include_directories(sivra-canonicalizer
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sivra-canonicalizer
  PUBLIC
    sivra::ir
)
