#include "../../support/graph_builder_fixture.hpp"

#include <sivra/ir/operation.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "built-in arithmetic operations retain their algebraic semantics"
) {
  const auto operations = sivra::test_support::make_test_catalogue();
  const auto& add = operations.catalogue->operation(operations.builtins.add);
  const auto& multiply = operations.catalogue->operation(operations.builtins.multiply);

  REQUIRE(add.evaluator_key().has_value());
  CHECK(*add.evaluator_key() == sivra::ir::operation_key("add"));
  CHECK(
    add.has_trait(sivra::ir::operation_trait::associative | sivra::ir::operation_trait::commutative)
  );
  REQUIRE(add.semantics().identity.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(add.semantics().identity->element) ==
    sivra::ir::well_known_constant::zero
  );

  REQUIRE(multiply.semantics().identity.has_value());
  REQUIRE(multiply.evaluator_key().has_value());
  CHECK(*multiply.evaluator_key() == sivra::ir::operation_key("multiply"));
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
  const auto& maximum = operations.catalogue->operation(operations.builtins.maximum);
  const auto& sqrt = operations.catalogue->operation(operations.builtins.sqrt);

  CHECK(add.signature().arity.minimum == 2);
  CHECK(!add.signature().arity.maximum.has_value());
  CHECK(subtract.signature().arity.minimum == 2);
  CHECK(subtract.signature().arity.maximum == 2);
  CHECK(maximum.signature().arity.minimum == 2);
  CHECK(!maximum.signature().arity.maximum.has_value());
  CHECK(sqrt.signature().arity.minimum == 1);
  CHECK(sqrt.signature().arity.maximum == 1);
}

TEST_CASE(
  "built-in min max and bitwise operations expose relaxed algebraic metadata"
) {
  const auto operations = sivra::test_support::make_test_catalogue();
  const auto& maximum = operations.catalogue->operation(operations.builtins.maximum);
  const auto& minimum = operations.catalogue->operation(operations.builtins.minimum);
  const auto& bit_and = operations.catalogue->operation(operations.builtins.bit_and);
  const auto& bit_or = operations.catalogue->operation(operations.builtins.bit_or);
  const auto& bit_xor = operations.catalogue->operation(operations.builtins.bit_xor);

  const auto associative_commutative_idempotent = sivra::ir::operation_trait::associative |
                                                  sivra::ir::operation_trait::commutative |
                                                  sivra::ir::operation_trait::idempotent;
  CHECK(maximum.has_trait(associative_commutative_idempotent));
  CHECK(minimum.has_trait(associative_commutative_idempotent));
  CHECK(bit_and.has_trait(associative_commutative_idempotent));
  CHECK(bit_or.has_trait(associative_commutative_idempotent));
  CHECK(bit_xor.has_trait(
    sivra::ir::operation_trait::associative | sivra::ir::operation_trait::commutative
  ));
  CHECK(!bit_xor.has_trait(sivra::ir::operation_trait::idempotent));
}

TEST_CASE(
  "built-in noncommutative operations retain directional constants"
) {
  const auto operations = sivra::test_support::make_test_catalogue();
  const auto& subtract = operations.catalogue->operation(operations.builtins.subtract);
  const auto& divide = operations.catalogue->operation(operations.builtins.divide);
  const auto& bit_and_not = operations.catalogue->operation(operations.builtins.bit_and_not);

  REQUIRE(subtract.semantics().right_identity.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(subtract.semantics().right_identity->element) ==
    sivra::ir::well_known_constant::zero
  );
  REQUIRE(divide.semantics().right_identity.has_value());
  CHECK(!divide.semantics().left_annihilator.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(divide.semantics().right_identity->element) ==
    sivra::ir::well_known_constant::one
  );
  REQUIRE(bit_and_not.semantics().left_identity.has_value());
  REQUIRE(bit_and_not.semantics().right_annihilator.has_value());
}
