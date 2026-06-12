#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/rewrite.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace {

sivra::canonicalizer::rewrite_result apply_oscillation(
  sivra::canonicalizer::rewrite_context& context,
  const sivra::canonicalizer::rewrite_subject& subject
) {
  if (context.operation(subject.operation).key() != "oscillate" || subject.operands.size() != 2) {
    return sivra::canonicalizer::no_match{};
  }
  auto operands = subject.operands;
  std::ranges::reverse(operands);
  return sivra::canonicalizer::rebuild_expression{
    .operation = subject.operation,
    .operands = std::move(operands),
    .attributes = subject.attributes,
    .result_type = subject.result_type,
  };
}

sivra::canonicalizer::rewrite_result apply_growth(
  sivra::canonicalizer::rewrite_context& context,
  const sivra::canonicalizer::rewrite_subject& subject
) {
  if (context.operation(subject.operation).key() != "grow") {
    return sivra::canonicalizer::no_match{};
  }
  auto constant = sivra::ir::constant_value::scalar(
    subject.result_type, sivra::ir::f32_constant::from_value(0.0F)
  );
  if (!constant.has_value()) {
    return sivra::canonicalizer::invalid_rewrite{
      .diagnostic = std::move(constant.error().front()),
    };
  }
  auto node = context.make_constant(std::move(*constant));
  if (!node.has_value()) {
    return sivra::canonicalizer::invalid_rewrite{
      .diagnostic = std::move(node.error().front()),
    };
  }
  auto operands = subject.operands;
  operands.push_back(*node);
  return sivra::canonicalizer::rebuild_expression{
    .operation = subject.operation,
    .operands = std::move(operands),
    .attributes = subject.attributes,
    .result_type = subject.result_type,
  };
}

std::shared_ptr<const sivra::canonicalizer::rule_catalogue> catalogue_with(
  sivra::canonicalizer::rewrite_rule rule
) {
  const auto builtins = sivra::canonicalizer::builtin_rule_catalogue();
  std::vector rules(builtins->rules().begin(), builtins->rules().end());
  rules.push_back(std::move(rule));
  return sivra::test_support::require_value(
    sivra::canonicalizer::rule_catalogue::create(std::move(rules))
  );
}

} // namespace

TEST_CASE(
  "worklist detects oscillating rewrite sequences"
) {
  auto operation = sivra::test_support::test_operation(
    "oscillate",
    {},
    {
      .arity = {.minimum = 2, .maximum = 2},
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto root =
    fixture.apply(fixture.operations.custom.front(), {fixture.symbol("a"), fixture.symbol("b")});
  const sivra::canonicalizer::rule_id rule_id("test.oscillation");
  auto rules = catalogue_with(
    {
      .metadata =
        {
          .id = rule_id,
          .name = "oscillation",
          .phase = sivra::canonicalizer::pass_phase::local_simplification,
          .priority = 50,
          .description = "Synthetic oscillation rule.",
          .decreasing_measure = "none",
        },
      .apply = apply_oscillation,
    }
  );
  sivra::canonicalizer::configuration configuration;
  REQUIRE(configuration.enable_rule(rule_id).has_value());
  const sivra::canonicalizer::engine engine(
    configuration, std::move(rules), sivra::canonicalizer::builtin_evaluator_catalogue()
  );

  const auto result = engine.canonicalize(fixture.graph, root);

  CHECK(result.status == sivra::core::analysis_status::resource_exhausted);
  REQUIRE(result.root.has_value());
  CHECK(result.graph.validate().has_value());
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code == "canonicalizer.oscillation");
}

TEST_CASE(
  "worklist growth budget retains the last valid graph"
) {
  auto operation = sivra::test_support::test_operation(
    "grow",
    {},
    {
      .arity = {.minimum = 2, .maximum = std::nullopt},
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto root =
    fixture.apply(fixture.operations.custom.front(), {fixture.symbol("a"), fixture.symbol("b")});
  const sivra::canonicalizer::rule_id rule_id("test.growth");
  auto rules = catalogue_with(
    {
      .metadata =
        {
          .id = rule_id,
          .name = "growth",
          .phase = sivra::canonicalizer::pass_phase::local_simplification,
          .priority = 50,
          .description = "Synthetic growth rule.",
          .decreasing_measure = "none",
          .may_grow = true,
        },
      .apply = apply_growth,
    }
  );
  sivra::canonicalizer::configuration configuration;
  REQUIRE(configuration.enable_rule(rule_id).has_value());
  configuration.set_collect_trace(true);
  auto limits = configuration.limits();
  limits.maximum_node_growth = 1;
  configuration.set_limits(limits);
  const sivra::canonicalizer::engine engine(
    configuration, std::move(rules), sivra::canonicalizer::builtin_evaluator_catalogue()
  );

  const auto result = engine.canonicalize(fixture.graph, root);

  CHECK(result.status == sivra::core::analysis_status::resource_exhausted);
  REQUIRE(result.root.has_value());
  CHECK(result.graph.validate().has_value());
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code.value() == "canonicalizer.growth_budget");
  CHECK(result.statistics.exhausted_budget);
  CHECK(result.trace.empty());
}
