#include "../../support/graph_builder_fixture.hpp"

#include <sivra/ir/operation.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "built-in arithmetic operations retain their algebraic semantics"
) {
  const auto operations = sivra::test_support::make_test_catalogue();
  const auto& add = operations.catalogue->operation(operations.builtins.add);
  const auto& multiply = operations.catalogue->operation(operations.builtins.multiply);

  CHECK(
    add.has_trait(sivra::ir::operation_trait::associative | sivra::ir::operation_trait::commutative)
  );
  REQUIRE(add.semantics().identity.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(add.semantics().identity->element) ==
    sivra::ir::well_known_constant::zero
  );

  REQUIRE(multiply.semantics().identity.has_value());
  REQUIRE(multiply.semantics().annihilator.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(multiply.semantics().identity->element) ==
    sivra::ir::well_known_constant::one
  );
  CHECK(
    std::get<sivra::ir::well_known_constant>(multiply.semantics().annihilator->element) ==
    sivra::ir::well_known_constant::zero
  );
}

TEST_CASE(
  "built-in signatures distinguish variadic and binary operations"
) {
  const auto operations = sivra::test_support::make_test_catalogue();
  const auto& add = operations.catalogue->operation(operations.builtins.add);
  const auto& subtract = operations.catalogue->operation(operations.builtins.subtract);

  CHECK(add.signature().arity.minimum == 2);
  CHECK(!add.signature().arity.maximum.has_value());
  CHECK(subtract.signature().arity.minimum == 2);
  CHECK(subtract.signature().arity.maximum == 2);
}
