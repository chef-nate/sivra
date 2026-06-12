#include "../../support/expression_format.hpp"
#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/engine.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "commutative ordering is deterministic"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root =
    fixture.apply(fixture.operations.builtins.add, {fixture.symbol("b"), fixture.symbol("a")});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  CHECK(sivra::test_support::format_expression(result.graph, *result.root) == "add(a, b)");
}

TEST_CASE(
  "idempotent operations remove structurally duplicate operands"
) {
  auto operation = sivra::test_support::test_operation(
    "choose",
    {
      .traits = sivra::ir::operation_trait::idempotent,
    },
    {
      .arity = {.minimum = 2, .maximum = std::nullopt},
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto value = fixture.symbol("x");
  const auto root = fixture.apply(fixture.operations.custom.front(), {value, value});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  CHECK(sivra::test_support::format_expression(result.graph, *result.root) == "x");
}
