set(SIVRA_EXAMPLE_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/fixtures/raw-expression-v1)

set(SIVRA_EXAMPLE_FILES
  ${SIVRA_EXAMPLE_DIRECTORY}/dot_product.json
  ${SIVRA_EXAMPLE_DIRECTORY}/weighted_clamp.json
  ${SIVRA_EXAMPLE_DIRECTORY}/mat4_transform_xmm0.json
  ${SIVRA_EXAMPLE_DIRECTORY}/mat4_transform_xmm1.json
  ${SIVRA_EXAMPLE_DIRECTORY}/mat4_transform_xmm2.json
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SIVRA_EXAMPLE_FILES})

file(READ "${SIVRA_EXAMPLE_DIRECTORY}/dot_product.json" SIVRA_DOT_PRODUCT_JSON)
file(READ "${SIVRA_EXAMPLE_DIRECTORY}/weighted_clamp.json" SIVRA_WEIGHTED_CLAMP_JSON)
file(READ
  "${SIVRA_EXAMPLE_DIRECTORY}/mat4_transform_xmm0.json"
  SIVRA_MAT4_TRANSFORM_XMM0_JSON
)
file(READ
  "${SIVRA_EXAMPLE_DIRECTORY}/mat4_transform_xmm1.json"
  SIVRA_MAT4_TRANSFORM_XMM1_JSON
)
file(READ
  "${SIVRA_EXAMPLE_DIRECTORY}/mat4_transform_xmm2.json"
  SIVRA_MAT4_TRANSFORM_XMM2_JSON
)

configure_file(
  embedded_examples.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/embedded_examples.cpp
  @ONLY
)

add_executable(sivra
  ${CMAKE_CURRENT_BINARY_DIR}/embedded_examples.cpp
  main.cpp
)

target_include_directories(sivra
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sivra
  PRIVATE
    sivra-compat-raw-expression-json
    sivra::canonicalizer
    CLI11::CLI11
)

target_compile_features(sivra
  PRIVATE
    cxx_std_23
)

set_target_properties(sivra
  PROPERTIES
    CXX_EXTENSIONS OFF
)
