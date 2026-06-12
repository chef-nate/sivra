set(SIVRA_EXAMPLE_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/fixtures/raw-expression-v1)

add_executable(sivra
  examples.cpp
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

target_compile_definitions(sivra
  PRIVATE
    SIVRA_RAW_EXPRESSION_FIXTURE_DIRECTORY="${SIVRA_EXAMPLE_DIRECTORY}"
)

target_compile_features(sivra
  PRIVATE
    cxx_std_23
)

set_target_properties(sivra
  PROPERTIES
    CXX_EXTENSIONS OFF
)

sivra_apply_build_options(sivra)
