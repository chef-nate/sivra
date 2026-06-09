#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/operation_registry.hpp>

#include <doctest/doctest.h>

#include <stdexcept>
#include <variant>

TEST_CASE(
  "register_builtin_operations registers standard operation names"
) {
  sivra::ir::operation_registry operations;
  const auto builtins = sivra::ir::register_builtin_operations(operations);

  CHECK(operations.at(builtins.constant).name() == "constant");
  CHECK(operations.at(builtins.symbol).name() == "symbol");
  CHECK(operations.at(builtins.memory_load).name() == "memory_load");
  CHECK(operations.at(builtins.add).name() == "add");
  CHECK(operations.at(builtins.multiply).name() == "multiply");

  CHECK(operations.at("constant").id() == builtins.constant);
  CHECK(operations.at("symbol").id() == builtins.symbol);
  CHECK(operations.at("memory_load").id() == builtins.memory_load);
  CHECK(operations.at("add").id() == builtins.add);
  CHECK(operations.at("multiply").id() == builtins.multiply);
}

TEST_CASE(
  "register_builtin_operations defines add semantics"
) {
  sivra::ir::operation_registry operations;
  const auto builtins = sivra::ir::register_builtin_operations(operations);

  const auto& add = operations.at(builtins.add);

  CHECK(add.has_trait(sivra::ir::operation_trait::associative));
  CHECK(add.has_trait(sivra::ir::operation_trait::commutative));
  REQUIRE(add.semantics().identity.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(add.semantics().identity->element) ==
    sivra::ir::well_known_constant::zero
  );
  CHECK(!add.semantics().annihilator.has_value());
}

TEST_CASE(
  "register_builtin_operations defines multiply semantics"
) {
  sivra::ir::operation_registry operations;
  const auto builtins = sivra::ir::register_builtin_operations(operations);

  const auto& multiply = operations.at(builtins.multiply);

  CHECK(multiply.has_trait(sivra::ir::operation_trait::associative));
  CHECK(multiply.has_trait(sivra::ir::operation_trait::commutative));
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
  "register_builtin_operations rejects duplicate registration"
) {
  sivra::ir::operation_registry operations;
  static_cast<void>(sivra::ir::register_builtin_operations(operations));

  CHECK_THROWS_AS(
    static_cast<void>(sivra::ir::register_builtin_operations(operations)), std::invalid_argument
  );
}
