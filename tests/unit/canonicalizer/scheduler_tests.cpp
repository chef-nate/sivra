#include "../../support/expression_format.hpp"
#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/engine.hpp>

#include <doctest/doctest.h>

#include <memory>
#include <utility>
#include <vector>

namespace {

sivra::canonicalizer::rewrite_result apply_validation_order(
  sivra::canonicalizer::rewrite_context& context,
  const sivra::canonicalizer::rewrite_subject& subject
) {
  if (context.operation(subject.operation).stable_key() !=
        sivra::ir::operation_key("validation_order") ||
      subject.operands.size() != 2 ||
      context.structural().compare(
        context.graph(), subject.operands[0], context.graph(), subject.operands[1]
      ) != std::strong_ordering::less) {
    return sivra::canonicalizer::no_match{};
  }
  return sivra::canonicalizer::rebuild_expression{
    .operation = subject.operation,
    .operands = {subject.operands[1], subject.operands[0]},
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
  "canonicalizer scheduler exposes deterministic typed phase plans"
) {
  const sivra::canonicalizer::engine engine;
  const auto plans = engine.scheduler().plans();

  REQUIRE(plans.size() == 7);
  CHECK(plans[0].phase == sivra::canonicalizer::pass_phase::validation);
  CHECK(plans[1].phase == sivra::canonicalizer::pass_phase::local_simplification);
  CHECK(plans[2].phase == sivra::canonicalizer::pass_phase::shape_normalization);
  REQUIRE(plans[1].rules.size() == 4);
  CHECK(plans[1].rules[0] == sivra::canonicalizer::builtin_rules::identity_elimination);
  CHECK(plans[1].rules[1] == sivra::canonicalizer::builtin_rules::annihilator_collapse);
  CHECK(plans[1].rules[2] == sivra::canonicalizer::builtin_rules::same_operand_simplification);
  CHECK(plans[1].rules[3] == sivra::canonicalizer::builtin_rules::constant_folding);
  REQUIRE(plans[2].rules.size() == 3);
  CHECK(plans[2].rules[0] == sivra::canonicalizer::builtin_rules::associative_flattening);
  CHECK(plans[2].rules[1] == sivra::canonicalizer::builtin_rules::commutative_ordering);
  CHECK(plans[2].rules[2] == sivra::canonicalizer::builtin_rules::idempotent_deduplication);
  REQUIRE(plans[3].rules.size() == 1);
  CHECK(plans[3].rules[0] == sivra::canonicalizer::builtin_rules::coefficient_collection);
  CHECK(engine.scheduler().validate(engine.rules()).has_value());
}

TEST_CASE(
  "rule catalogue rejects duplicate stable identifiers"
) {
  auto rules = std::vector<sivra::canonicalizer::rewrite_rule>(
    sivra::canonicalizer::builtin_rule_catalogue()->rules().begin(),
    sivra::canonicalizer::builtin_rule_catalogue()->rules().end()
  );
  rules.push_back(rules.front());

  const auto catalogue = sivra::canonicalizer::rule_catalogue::create(std::move(rules));

  REQUIRE(!catalogue.has_value());
  CHECK(catalogue.error().front().code == "canonicalizer.rule_catalogue.duplicate_rule");
}

TEST_CASE(
  "single-iteration phases do not prevent later phases from running"
) {
  auto operation = sivra::test_support::test_operation(
    "validation_order",
    {},
    {
      .arity = {.minimum = 2, .maximum = 2},
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto ordered =
    fixture.apply(fixture.operations.custom.front(), {fixture.symbol("a"), fixture.symbol("b")});
  const auto root = fixture.apply(fixture.operations.builtins.add, {ordered, fixture.f32(0.0F)});
  const sivra::canonicalizer::rule_id rule_id("test.validation_order");
  auto rules = catalogue_with(
    {
      .metadata =
        {
          .id = rule_id,
          .name = "validation_order",
          .phase = sivra::canonicalizer::pass_phase::validation,
          .priority = 100,
          .description = "Order a synthetic validation expression.",
          .decreasing_measure = "operand order",
        },
      .apply = apply_validation_order,
    }
  );
  sivra::canonicalizer::configuration configuration;
  REQUIRE(configuration.enable_rule(rule_id).has_value());
  const sivra::canonicalizer::engine engine(
    configuration, std::move(rules), sivra::canonicalizer::builtin_evaluator_catalogue()
  );

  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  CHECK(result.status == sivra::core::analysis_status::complete);
  CHECK(
    sivra::test_support::format_expression(result.graph, *result.root) == "validation_order(b, a)"
  );
}

TEST_CASE(
  "rule catalogues reject unknown phases"
) {
  const auto catalogue = sivra::canonicalizer::rule_catalogue::create(
    {
      {
        .metadata =
          {
            .id = sivra::canonicalizer::rule_id("test.invalid_phase"),
            .name = "invalid_phase",
            .phase = static_cast<sivra::canonicalizer::pass_phase>(999),
            .description = "Invalid synthetic phase.",
            .decreasing_measure = "none",
          },
        .apply = apply_validation_order,
      },
    }
  );

  REQUIRE(!catalogue.has_value());
  CHECK(catalogue.error().front().code == "canonicalizer.rule_catalogue.invalid_phase");
}
