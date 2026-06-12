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
  "canonicalizer configuration rejects unknown rule identifiers"
) {
  sivra::canonicalizer::configuration configuration;
  const sivra::canonicalizer::rule_id unknown("test.unknown");

  const auto enabled = configuration.enable_rule(unknown);
  const auto disabled = configuration.disable_rule(unknown);

  REQUIRE(!enabled.has_value());
  REQUIRE(!disabled.has_value());
  REQUIRE(enabled.error().size() == 1);
  REQUIRE(disabled.error().size() == 1);
  CHECK(enabled.error().front().code == "canonicalizer.configuration.unknown_rule");
  CHECK(configuration.validate().has_value());
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

  REQUIRE(rules.size() == 3);
  CHECK(rules[0].id.key() == "sivra.associative_flattening");
  CHECK(rules[1].id.key() == "sivra.identity_elimination");
  CHECK(rules[2].id.key() == "sivra.annihilator_collapse");
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
      .maximum_worklist_steps = 30,
      .maximum_rewrites = 40,
    }
  );

  const sivra::canonicalizer::engine engine(configuration);

  CHECK(!engine.configuration().is_trait_enabled(sivra::ir::operation_trait::commutative));
  CHECK(engine.configuration().collect_trace());
  CHECK(engine.configuration().limits().maximum_rewrites == 40);
  CHECK(
    engine.configuration().contract() == sivra::canonicalizer::algebraic_equivalence_contract()
  );
}
