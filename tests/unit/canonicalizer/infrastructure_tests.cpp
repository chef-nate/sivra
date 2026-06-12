#include "../../support/expression_format.hpp"
#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/configuration.hpp>
#include <sivra/canonicalizer/engine.hpp>
#include <sivra/ir/structural.hpp>

#include <doctest/doctest.h>

#include <vector>

TEST_CASE(
  "canonicalizer reports its algebraic contract, statistics, and source mapping"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto symbol = fixture.symbol("x");
  const auto root = fixture.apply(fixture.operations.builtins.add, {symbol, fixture.f32(0.0F)});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  CHECK(result.status == sivra::core::analysis_status::complete);
  CHECK(result.contract == sivra::canonicalizer::algebraic_equivalence_contract());
  CHECK(result.statistics.imported_nodes == 3);
  CHECK(result.statistics.output_nodes == 1);
  CHECK(result.statistics.rewrites_applied == 1);
  CHECK(result.statistics.phase_iterations >= 2);
  CHECK(result.mapping.canonical_for(root) == result.root);
  CHECK(result.mapping.canonical_for(symbol) == result.root);
}

TEST_CASE(
  "canonicalization is structurally deterministic and idempotent"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto nested =
    fixture.apply(fixture.operations.builtins.add, {fixture.symbol("b"), fixture.symbol("c")});
  const auto root = fixture.apply(fixture.operations.builtins.add, {fixture.symbol("a"), nested});
  const sivra::canonicalizer::engine engine;

  const auto first = engine.canonicalize(fixture.graph, root);
  REQUIRE(first.root.has_value());
  const auto second = engine.canonicalize(first.graph, *first.root);
  REQUIRE(second.root.has_value());
  sivra::ir::structural_context structural;

  CHECK(structural.equal(first.graph, *first.root, second.graph, *second.root));
  CHECK(first.statistics.rewrites_applied == 1);
  CHECK(second.statistics.rewrites_applied == 0);
}

TEST_CASE(
  "canonicalization revisits earlier phases until the full phase sequence is stable"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto positive = fixture.apply(fixture.operations.builtins.multiply, {fixture.f32(1.0F), x});
  const auto negative =
    fixture.apply(fixture.operations.builtins.multiply, {fixture.f32(-1.0F), x});
  const auto root = fixture.apply(fixture.operations.builtins.add, {positive, negative});
  const sivra::canonicalizer::engine engine;

  const auto first = engine.canonicalize(fixture.graph, root);
  REQUIRE(first.status == sivra::core::analysis_status::complete);
  REQUIRE(first.root.has_value());
  const auto second = engine.canonicalize(first.graph, *first.root);
  REQUIRE(second.status == sivra::core::analysis_status::complete);
  REQUIRE(second.root.has_value());
  sivra::ir::structural_context structural;

  CHECK(structural.equal(first.graph, *first.root, second.graph, *second.root));
  CHECK(sivra::test_support::format_expression(first.graph, *first.root) == "0");
  CHECK(second.statistics.rewrites_applied == 0);
}

TEST_CASE(
  "canonicalizer collects deterministic optional traces"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root = fixture.apply(
    fixture.operations.builtins.multiply,
    {fixture.symbol("x"), fixture.f32(0.0F), fixture.f32(1.0F)}
  );
  sivra::canonicalizer::configuration configuration;
  configuration.set_collect_trace(true);

  const sivra::canonicalizer::engine engine(configuration);
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.trace.size() == 2);
  CHECK(result.trace[0].sequence == 0);
  CHECK(result.trace[0].rule == sivra::canonicalizer::builtin_rules::identity_elimination);
  CHECK(result.trace[0].phase == sivra::canonicalizer::pass_phase::local_simplification);
  CHECK(result.trace[1].sequence == 1);
  CHECK(result.trace[1].rule == sivra::canonicalizer::builtin_rules::annihilator_collapse);
  CHECK(result.trace[1].phase == sivra::canonicalizer::pass_phase::local_simplification);
}

TEST_CASE(
  "canonicalizer returns a valid partial artifact when rewrite budget is exhausted"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root =
    fixture.apply(fixture.operations.builtins.add, {fixture.symbol("x"), fixture.f32(0.0F)});
  sivra::canonicalizer::configuration configuration;
  auto limits = configuration.limits();
  limits.maximum_rewrites = 0;
  configuration.set_limits(limits);

  const sivra::canonicalizer::engine engine(configuration);
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  CHECK(result.status == sivra::core::analysis_status::resource_exhausted);
  CHECK(result.graph.validate().has_value());
  CHECK(sivra::test_support::format_expression(result.graph, *result.root) == "add(x, 0)");
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code == "canonicalizer.rewrite_budget");
}

TEST_CASE(
  "canonicalizer uses iterative import for deep expression graphs"
) {
  sivra::test_support::graph_builder_fixture fixture;
  auto root = fixture.symbol("x");
  for (int index = 0; index < 5'000; ++index) {
    root = fixture.apply(
      fixture.operations.builtins.maximum, {root, fixture.f32(static_cast<float>(index))}
    );
  }

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  CHECK(result.status == sivra::core::analysis_status::complete);
  CHECK(result.statistics.imported_nodes == fixture.graph.size());
  CHECK(result.graph.validate().has_value());
}
