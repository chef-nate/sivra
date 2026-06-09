#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/options.hpp>
#include <sivra/canonicalizer/rule.hpp>
#include <sivra/ir/operation.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "canonicalizer options enable rules individually and in combined masks"
) {
  sivra::canonicalizer::options options;
  const auto combined = sivra::canonicalizer::rule::associative_flattening |
                        sivra::canonicalizer::rule::identity_elimination |
                        sivra::canonicalizer::rule::annihilator_collapse;

  CHECK(options.is_rule_enabled(sivra::canonicalizer::rule::associative_flattening));
  CHECK(options.is_rule_enabled(sivra::canonicalizer::rule::identity_elimination));
  CHECK(options.is_rule_enabled(sivra::canonicalizer::rule::annihilator_collapse));
  CHECK(options.is_rule_enabled(combined));

  options.disable_rule(sivra::canonicalizer::rule::associative_flattening);
  CHECK(!options.is_rule_enabled(sivra::canonicalizer::rule::associative_flattening));
  CHECK(options.is_rule_enabled(sivra::canonicalizer::rule::identity_elimination));
  CHECK(options.is_rule_enabled(sivra::canonicalizer::rule::annihilator_collapse));
  CHECK(!options.is_rule_enabled(combined));

  options.enable_rule(sivra::canonicalizer::rule::associative_flattening);
  CHECK(options.is_rule_enabled(combined));

  options.disable_rule(combined);
  CHECK(!options.is_rule_enabled(sivra::canonicalizer::rule::associative_flattening));
  CHECK(!options.is_rule_enabled(sivra::canonicalizer::rule::identity_elimination));
  CHECK(!options.is_rule_enabled(sivra::canonicalizer::rule::annihilator_collapse));

  options.enable_rule(combined);
  CHECK(options.is_rule_enabled(combined));
}

TEST_CASE(
  "canonicalizer options enable traits individually and in combined masks"
) {
  sivra::canonicalizer::options options;
  const auto combined =
    sivra::ir::operation_trait::associative | sivra::ir::operation_trait::commutative;

  CHECK(options.is_trait_enabled(sivra::ir::operation_trait::associative));
  CHECK(options.is_trait_enabled(sivra::ir::operation_trait::commutative));
  CHECK(options.is_trait_enabled(sivra::ir::operation_trait::idempotent));
  CHECK(options.is_trait_enabled(combined));

  options.disable_trait(sivra::ir::operation_trait::commutative);
  CHECK(options.is_trait_enabled(sivra::ir::operation_trait::associative));
  CHECK(!options.is_trait_enabled(sivra::ir::operation_trait::commutative));
  CHECK(!options.is_trait_enabled(combined));

  options.enable_trait(sivra::ir::operation_trait::commutative);
  CHECK(options.is_trait_enabled(combined));

  options.disable_trait(combined);
  CHECK(!options.is_trait_enabled(sivra::ir::operation_trait::associative));
  CHECK(!options.is_trait_enabled(sivra::ir::operation_trait::commutative));
  CHECK(options.is_trait_enabled(sivra::ir::operation_trait::idempotent));

  options.enable_trait(combined);
  CHECK(options.is_trait_enabled(combined));
}

TEST_CASE(
  "canonicalizer engine preserves its configured options"
) {
  sivra::canonicalizer::options options;
  options.disable_rule(sivra::canonicalizer::rule::annihilator_collapse);
  options.disable_trait(sivra::ir::operation_trait::commutative);

  const sivra::canonicalizer::engine engine(options);

  CHECK(engine.config().enabled_rules == options.enabled_rules);
  CHECK(engine.config().enabled_traits == options.enabled_traits);
}
