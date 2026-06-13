#include <sivra/canonicalizer/configuration.hpp>
#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/rule.hpp>
#include <sivra/ir/operation.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "canonicalizer configuration uses stable rule identifiers"
) {
  sivra::canonicalizer::configuration configuration;

  CHECK(configuration.is_rule_enabled(sivra::canonicalizer::builtin_rules::associative_flattening));
  CHECK(configuration.is_rule_enabled(sivra::canonicalizer::builtin_rules::identity_elimination));
  CHECK(configuration.is_rule_enabled(sivra::canonicalizer::builtin_rules::annihilator_collapse));

  REQUIRE(configuration.disable_rule(sivra::canonicalizer::builtin_rules::identity_elimination)
            .has_value());
  CHECK(!configuration.is_rule_enabled(sivra::canonicalizer::builtin_rules::identity_elimination));
  REQUIRE(
    configuration.enable_rule(sivra::canonicalizer::builtin_rules::identity_elimination).has_value()
  );
  CHECK(configuration.is_rule_enabled(sivra::canonicalizer::builtin_rules::identity_elimination));
}

TEST_CASE(
  "canonicalizer configuration accepts dynamic stable rule identifiers"
) {
  sivra::canonicalizer::configuration configuration;
  const sivra::canonicalizer::rule_id custom("test.custom");

  REQUIRE(configuration.enable_rule(custom).has_value());
  CHECK(configuration.is_rule_enabled(custom));
  REQUIRE(configuration.disable_rule(custom).has_value());
  CHECK(!configuration.is_rule_enabled(custom));

  CHECK(configuration.validate().has_value());
}

TEST_CASE(
  "canonicalizer configuration controls explicit phases"
) {
  sivra::canonicalizer::configuration configuration;

  CHECK(configuration.is_phase_enabled(sivra::canonicalizer::pass_phase::shape_normalization));
  configuration.disable_phase(sivra::canonicalizer::pass_phase::shape_normalization);
  CHECK(!configuration.is_phase_enabled(sivra::canonicalizer::pass_phase::shape_normalization));
  configuration.enable_phase(sivra::canonicalizer::pass_phase::shape_normalization);
  CHECK(configuration.is_phase_enabled(sivra::canonicalizer::pass_phase::shape_normalization));
}

TEST_CASE(
  "canonicalizer configuration rejects unknown phases"
) {
  sivra::canonicalizer::configuration configuration;
  configuration.enable_phase(static_cast<sivra::canonicalizer::pass_phase>(999));

  const auto validated = configuration.validate();

  REQUIRE(!validated.has_value());
  CHECK(validated.error().front().code == "canonicalizer.configuration.invalid_phase");
}

TEST_CASE(
  "canonicalizer configuration controls operation traits"
) {
  sivra::canonicalizer::configuration configuration;
  const auto combined =
    sivra::ir::operation_trait::associative | sivra::ir::operation_trait::commutative;

  CHECK(configuration.is_trait_enabled(combined));
  configuration.disable_trait(sivra::ir::operation_trait::commutative);
  CHECK(!configuration.is_trait_enabled(combined));
  configuration.enable_trait(sivra::ir::operation_trait::commutative);
  CHECK(configuration.is_trait_enabled(combined));
}

TEST_CASE(
  "rule metadata is stable and separate from execution scheduling"
) {
  const auto rules = sivra::canonicalizer::available_rules();

  REQUIRE(rules.size() == 14);
  CHECK(rules[0].id.key() == "sivra.copy_elimination");
  CHECK(rules[1].id.key() == "sivra.associative_flattening");
  CHECK(rules[2].id.key() == "sivra.commutative_ordering");
  CHECK(rules[3].id.key() == "sivra.idempotent_deduplication");
  CHECK(rules[4].id.key() == "sivra.identity_elimination");
  CHECK(rules[5].id.key() == "sivra.annihilator_collapse");
  CHECK(rules[6].id.key() == "sivra.same_operand_simplification");
  CHECK(rules[7].id.key() == "sivra.constant_folding");
  CHECK(rules[8].id.key() == "sivra.mixed_constant_aggregation");
  CHECK(rules[9].id.key() == "sivra.subtraction_normalization");
  CHECK(rules[10].id.key() == "sivra.coefficient_collection");
  CHECK(rules[11].id.key() == "sivra.division_reciprocal_simplification");
  CHECK(rules[12].id.key() == "sivra.bitwise_simplification");
  CHECK(rules[13].id.key() == "sivra.square_simplification");
  CHECK(rules[0].enabled_by_default);
}

TEST_CASE(
  "canonicalizer engine preserves its validated configuration"
) {
  sivra::canonicalizer::configuration configuration;
  configuration.disable_trait(sivra::ir::operation_trait::commutative);
  configuration.set_collect_trace(true);
  configuration.set_limits(
    {
      .maximum_imported_nodes = 10,
      .maximum_output_nodes = 20,
      .maximum_node_growth = 15,
      .maximum_worklist_steps = 30,
      .maximum_rewrites = 40,
      .maximum_rewrites_per_node = 8,
      .maximum_phase_iterations = 12,
    }
  );

  const sivra::canonicalizer::engine engine(configuration);

  CHECK(!engine.configuration().is_trait_enabled(sivra::ir::operation_trait::commutative));
  CHECK(engine.configuration().collect_trace());
  CHECK(engine.configuration().limits().maximum_rewrites == 40);
  CHECK(engine.configuration().limits().maximum_phase_iterations == 12);
  CHECK(
    engine.configuration().contract() == sivra::canonicalizer::algebraic_equivalence_contract()
  );
}
