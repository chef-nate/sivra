#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/evaluator.hpp>

#include <doctest/doctest.h>

#include <array>
#include <variant>

TEST_CASE(
  "built-in evaluator catalogue resolves stable operation keys"
) {
  const auto catalogue = sivra::canonicalizer::builtin_evaluator_catalogue();

  CHECK(catalogue->find(sivra::ir::operation_key("add")) != nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("add", 2)) == nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("unknown")) == nullptr);
}

TEST_CASE(
  "built-in evaluator folds exact i32 constants"
) {
  const auto catalogue = sivra::canonicalizer::builtin_evaluator_catalogue();
  const auto* evaluator = catalogue->find(sivra::ir::operation_key("multiply"));
  REQUIRE(evaluator != nullptr);
  const std::array operands{
    sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::i32(), sivra::ir::i32_constant::from_value(6)
      )
    ),
    sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::i32(), sivra::ir::i32_constant::from_value(7)
      )
    ),
  };

  const auto result =
    evaluator->evaluate(operands, sivra::ir::operation_attributes{}, sivra::ir::value_type::i32());

  REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(result));
  const auto& value = std::get<sivra::canonicalizer::evaluated_constant>(result).value;
  CHECK(std::get<sivra::ir::i32_constant>(value.element(0)).value() == 42);
}

TEST_CASE(
  "floating maximum treats positive and negative zero as the same mathematical zero"
) {
  const auto catalogue = sivra::canonicalizer::builtin_evaluator_catalogue();
  const auto* evaluator = catalogue->find(sivra::ir::operation_key("maximum"));
  REQUIRE(evaluator != nullptr);
  const std::array forward{
    sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::f32(), sivra::ir::f32_constant{.bits = 0x0000'0000U}
      )
    ),
    sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::f32(), sivra::ir::f32_constant{.bits = 0x8000'0000U}
      )
    ),
  };
  const std::array reverse{forward[1], forward[0]};

  const auto first =
    evaluator->evaluate(forward, sivra::ir::operation_attributes{}, sivra::ir::value_type::f32());
  const auto second =
    evaluator->evaluate(reverse, sivra::ir::operation_attributes{}, sivra::ir::value_type::f32());

  REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(first));
  REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(second));
  const auto& first_value = std::get<sivra::canonicalizer::evaluated_constant>(first).value;
  const auto& second_value = std::get<sivra::canonicalizer::evaluated_constant>(second).value;
  CHECK(std::get<sivra::ir::f32_constant>(first_value.element(0)).bits == 0x0000'0000U);
  CHECK(first_value == second_value);
}

TEST_CASE(
  "commutative floating folds are deterministic across operand permutations"
) {
  const auto catalogue = sivra::canonicalizer::builtin_evaluator_catalogue();
  const auto* evaluator = catalogue->find(sivra::ir::operation_key("add"));
  REQUIRE(evaluator != nullptr);
  const std::array first_operands{
    sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(1.0e20F)
      )
    ),
    sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(-1.0e20F)
      )
    ),
    sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(3.0F)
      )
    ),
  };
  const std::array second_operands{first_operands[2], first_operands[0], first_operands[1]};

  const auto first = evaluator->evaluate(
    first_operands, sivra::ir::operation_attributes{}, sivra::ir::value_type::f32()
  );
  const auto second = evaluator->evaluate(
    second_operands, sivra::ir::operation_attributes{}, sivra::ir::value_type::f32()
  );

  REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(first));
  REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(second));
  CHECK(
    std::get<sivra::canonicalizer::evaluated_constant>(first).value ==
    std::get<sivra::canonicalizer::evaluated_constant>(second).value
  );
}
