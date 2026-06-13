#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/evaluator.hpp>

#include <doctest/doctest.h>

#include <array>
#include <utility>
#include <variant>

TEST_CASE(
  "built-in evaluator catalogue resolves stable operation keys"
) {
  const auto catalogue = sivra::canonicalizer::builtin_evaluator_catalogue();

  CHECK(catalogue->find(sivra::ir::operation_key("add")) != nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("divide")) != nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("minimum")) != nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("reciprocal_sqrt")) != nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("bit_and_not")) != nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("copy")) != nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("add", 2)) == nullptr);
  CHECK(catalogue->find(sivra::ir::operation_key("unknown")) == nullptr);
}

TEST_CASE(
  "engine rejects operation catalogues with unbound evaluator keys"
) {
  auto operation = sivra::test_support::test_operation("custom_fold");
  operation.semantics.evaluator_key = sivra::ir::operation_key("missing_evaluator");
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto root =
    fixture.apply(fixture.operations.custom.front(), {fixture.f32(1.0F), fixture.f32(2.0F)});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  CHECK(result.status == sivra::core::analysis_status::invalid_input);
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code == "canonicalizer.evaluator_catalogue.missing_evaluator");
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

TEST_CASE(
  "floating evaluators fold divide minimum and unary operations"
) {
  const auto catalogue = sivra::canonicalizer::builtin_evaluator_catalogue();
  const auto constant = [](float value) {
    return sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(value)
      )
    );
  };
  const auto evaluate =
    [&](std::string_view operation, std::span<const sivra::ir::constant_value> operands) {
      const auto* evaluator = catalogue->find(sivra::ir::operation_key(std::string(operation)));
      REQUIRE(evaluator != nullptr);
      const auto result = evaluator->evaluate(
        operands, sivra::ir::operation_attributes{}, sivra::ir::value_type::f32()
      );
      REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(result));
      return std::get<sivra::ir::f32_constant>(
               std::get<sivra::canonicalizer::evaluated_constant>(result).value.element(0)
      )
        .value();
    };

  const std::array divide_operands{constant(9.0F), constant(4.0F)};
  const std::array minimum_operands{constant(7.0F), constant(3.0F), constant(5.0F)};
  const std::array sqrt_operand{constant(9.0F)};
  const std::array reciprocal_operand{constant(4.0F)};
  const std::array reciprocal_sqrt_operand{constant(4.0F)};
  const std::array square_operand{constant(3.0F)};

  CHECK(evaluate("divide", divide_operands) == doctest::Approx(2.25F));
  CHECK(evaluate("minimum", minimum_operands) == doctest::Approx(3.0F));
  CHECK(evaluate("sqrt", sqrt_operand) == doctest::Approx(3.0F));
  CHECK(evaluate("reciprocal", reciprocal_operand) == doctest::Approx(0.25F));
  CHECK(evaluate("reciprocal_sqrt", reciprocal_sqrt_operand) == doctest::Approx(0.5F));
  CHECK(evaluate("square", square_operand) == doctest::Approx(9.0F));
}

TEST_CASE(
  "bitwise and copy evaluators preserve exact stored bits"
) {
  const auto catalogue = sivra::canonicalizer::builtin_evaluator_catalogue();
  const auto bits = [](std::uint32_t value) {
    return sivra::test_support::require_value(
      sivra::ir::constant_value::scalar(
        sivra::ir::value_type::f32(), sivra::ir::f32_constant{.bits = value}
      )
    );
  };
  const auto* and_not = catalogue->find(sivra::ir::operation_key("bit_and_not"));
  const auto* copy = catalogue->find(sivra::ir::operation_key("copy"));
  REQUIRE(and_not != nullptr);
  REQUIRE(copy != nullptr);

  const std::array and_not_operands{bits(0x0F0F'0F0FU), bits(0xFFFF'0000U)};
  const auto and_not_result = and_not->evaluate(
    and_not_operands, sivra::ir::operation_attributes{}, sivra::ir::value_type::f32()
  );
  REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(and_not_result));
  CHECK(
    std::get<sivra::ir::f32_constant>(
      std::get<sivra::canonicalizer::evaluated_constant>(and_not_result).value.element(0)
    )
      .bits == 0xF0F0'0000U
  );

  const std::array copy_operand{bits(0x7FA1'2345U)};
  const auto copy_result =
    copy->evaluate(copy_operand, sivra::ir::operation_attributes{}, sivra::ir::value_type::f32());
  REQUIRE(std::holds_alternative<sivra::canonicalizer::evaluated_constant>(copy_result));
  CHECK(
    std::get<sivra::ir::f32_constant>(
      std::get<sivra::canonicalizer::evaluated_constant>(copy_result).value.element(0)
    )
      .bits == 0x7FA1'2345U
  );
}
