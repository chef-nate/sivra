#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/engine.hpp>

#include <doctest/doctest.h>

#include <array>
#include <stdexcept>

TEST_CASE(
  "canonicalizer rebuilds requested roots in order"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto lhs = fixture.symbol("lhs");
  const auto rhs = fixture.symbol("rhs");
  const auto sum = fixture.apply(fixture.operations.builtins.add, {lhs, rhs});
  const std::array roots{rhs, sum, lhs};

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, roots);

  REQUIRE(result.roots.size() == 3);
  CHECK(result.graph.at(result.roots[0]).get_if_symbol()->name == "rhs");
  CHECK(result.graph.at(result.roots[2]).get_if_symbol()->name == "lhs");
  REQUIRE(result.graph.at(result.roots[1]).get_if_operation() != nullptr);
}

TEST_CASE(
  "canonicalizer preserves duplicate roots and shared children"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto shared = fixture.symbol("shared");
  const auto lhs = fixture.apply(fixture.operations.builtins.subtract, {shared, fixture.f32(1.0F)});
  const auto rhs = fixture.apply(fixture.operations.builtins.maximum, {shared, fixture.f32(2.0F)});
  const auto root = fixture.apply(fixture.operations.builtins.add, {lhs, rhs});
  const std::array roots{root, root};

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, roots);

  CHECK(result.roots[0] == result.roots[1]);
  const auto root_operands = result.graph.at(result.roots[0]).operands();
  REQUIRE(root_operands.size() == 2);
  const auto lhs_operands = result.graph.at(root_operands[0]).operands();
  const auto rhs_operands = result.graph.at(root_operands[1]).operands();
  CHECK(lhs_operands[0] == rhs_operands[0]);
}

TEST_CASE(
  "canonicalizer output owns new nodes and shares the immutable catalogue"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root = fixture.symbol("x");

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  CHECK(result.graph.owner() != fixture.graph.owner());
  CHECK(result.root.owner() == result.graph.owner());
  CHECK(result.graph.shared_catalogue() == fixture.graph.shared_catalogue());
  CHECK(result.graph.at(result.root).get_if_symbol()->name == "x");
}

TEST_CASE(
  "canonicalizer copies every leaf variant"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto constant = fixture.f32(3.0F);
  const auto symbol = fixture.symbol("x");
  const auto external = sivra::test_support::require_value(
    fixture.builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto unknown = sivra::test_support::require_value(
    fixture.builder.make_unknown("unknown source", sivra::ir::value_type::f32())
  );
  const std::array roots{constant, symbol, external, unknown};

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, roots);

  CHECK(result.graph.at(result.roots[0]).get_if_constant() != nullptr);
  CHECK(result.graph.at(result.roots[1]).get_if_symbol() != nullptr);
  CHECK(result.graph.at(result.roots[2]).get_if_external_value() != nullptr);
  CHECK(result.graph.at(result.roots[3]).get_if_unknown() != nullptr);
}

TEST_CASE(
  "canonicalizer rejects roots owned by another graph"
) {
  sivra::test_support::graph_builder_fixture fixture;
  sivra::test_support::graph_builder_fixture foreign;
  const auto foreign_root = foreign.symbol("foreign");

  const sivra::canonicalizer::engine engine;
  CHECK_THROWS_AS(
    static_cast<void>(engine.canonicalize(fixture.graph, foreign_root)), std::invalid_argument
  );
}
