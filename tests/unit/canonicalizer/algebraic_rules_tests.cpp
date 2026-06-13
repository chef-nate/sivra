#include "../../support/expression_format.hpp"
#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/configuration.hpp>
#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/rule.hpp>

#include <doctest/doctest.h>

#include <array>
#include <limits>
#include <string>

namespace {

std::string canonical_expression(
  const sivra::test_support::graph_builder_fixture& fixture,
  sivra::ir::node_id root,
  sivra::canonicalizer::configuration configuration = {}
) {
  const sivra::canonicalizer::engine engine(std::move(configuration));
  const auto result = engine.canonicalize(fixture.graph, root);
  REQUIRE(result.status == sivra::core::analysis_status::complete);
  REQUIRE(result.root.has_value());
  return sivra::test_support::format_expression(result.graph, *result.root);
}

} // namespace

TEST_CASE(
  "associative operations flatten nested applications"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto a = fixture.symbol("a");
  const auto b = fixture.symbol("b");
  const auto c = fixture.symbol("c");
  const auto nested = fixture.apply(fixture.operations.builtins.add, {b, c});
  const auto root = fixture.apply(fixture.operations.builtins.add, {a, nested});

  CHECK(canonical_expression(fixture, root) == "add(a, b, c)");
}

TEST_CASE(
  "associative flattening respects the enabled trait mask"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto a = fixture.symbol("a");
  const auto b = fixture.symbol("b");
  const auto c = fixture.symbol("c");
  const auto nested = fixture.apply(fixture.operations.builtins.add, {b, c});
  const auto root = fixture.apply(fixture.operations.builtins.add, {a, nested});
  sivra::canonicalizer::configuration configuration;
  configuration.disable_trait(sivra::ir::operation_trait::associative);

  CHECK(canonical_expression(fixture, root, configuration) == "add(a, add(b, c))");
}

TEST_CASE(
  "flattening stops at a different operation or result type"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto a = fixture.symbol("a");
  const auto b = fixture.symbol("b");
  const auto c = fixture.symbol("c");
  const auto nested = fixture.apply(fixture.operations.builtins.multiply, {b, c});
  const auto root = fixture.apply(fixture.operations.builtins.add, {a, nested});

  CHECK(canonical_expression(fixture, root) == "add(a, multiply(b, c))");
}

TEST_CASE(
  "flattening stops when nested operation attributes differ"
) {
  const auto schema = sivra::test_support::require_value(
    sivra::ir::operation_attribute_schema::create(
      std::array{
        sivra::ir::operation_attribute_field{
          .key = "lane",
          .kind = sivra::ir::operation_attribute_kind::integer,
          .required = true,
        },
      }
    )
  );
  auto operation = sivra::test_support::test_operation(
    "tagged",
    {
      .traits = sivra::ir::operation_trait::associative,
    },
    {
      .arity =
        {
          .minimum = 2,
          .maximum = std::nullopt,
        },
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  operation.attribute_schema = schema;
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto custom = fixture.operations.custom.front();
  const auto child_attributes = sivra::test_support::require_value(
    sivra::ir::operation_attributes::create(
      std::array{
        sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{1}},
      }
    )
  );
  const auto parent_attributes = sivra::test_support::require_value(
    sivra::ir::operation_attributes::create(
      std::array{
        sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{2}},
      }
    )
  );
  const auto b = fixture.symbol("b");
  const auto c = fixture.symbol("c");
  const std::array child_operands{b, c};
  const auto nested = sivra::test_support::require_value(
    fixture.builder.apply(custom, child_operands, child_attributes, sivra::ir::value_type::f32())
  );
  const std::array parent_operands{fixture.symbol("a"), nested};
  const auto root = sivra::test_support::require_value(
    fixture.builder.apply(custom, parent_operands, parent_attributes, sivra::ir::value_type::f32())
  );

  CHECK(canonical_expression(fixture, root) == "tagged(a, tagged(b, c))");
}

TEST_CASE(
  "identity elimination handles add and multiply"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto add = fixture.apply(fixture.operations.builtins.add, {x, fixture.f32(0.0F)});
  const auto multiply = fixture.apply(fixture.operations.builtins.multiply, {x, fixture.f32(1.0F)});

  CHECK(canonical_expression(fixture, add) == "x");
  CHECK(canonical_expression(fixture, multiply) == "x");
}

TEST_CASE(
  "identity elimination retains one value when every operand is an identity"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root =
    fixture.apply(fixture.operations.builtins.multiply, {fixture.f32(1.0F), fixture.f32(1.0F)});

  CHECK(canonical_expression(fixture, root) == "1");
}

TEST_CASE(
  "annihilator collapse replaces a multiplication with zero"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root = fixture.apply(
    fixture.operations.builtins.multiply,
    {fixture.symbol("x"), fixture.f32(0.0F), fixture.symbol("y")}
  );

  CHECK(canonical_expression(fixture, root) == "0");
}

TEST_CASE(
  "identity elimination feeds annihilator collapse"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root = fixture.apply(
    fixture.operations.builtins.multiply,
    {fixture.f32(1.0F), fixture.symbol("x"), fixture.f32(0.0F), fixture.f32(1.0F)}
  );

  CHECK(canonical_expression(fixture, root) == "0");
}

TEST_CASE(
  "disabled rules preserve their part of the expression"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root = fixture.apply(
    fixture.operations.builtins.multiply,
    {fixture.symbol("x"), fixture.f32(0.0F), fixture.f32(1.0F)}
  );
  sivra::canonicalizer::configuration configuration;
  REQUIRE(configuration.disable_rule(sivra::canonicalizer::builtin_rules::identity_elimination)
            .has_value());
  REQUIRE(configuration.disable_rule(sivra::canonicalizer::builtin_rules::annihilator_collapse)
            .has_value());
  REQUIRE(configuration
            .disable_rule(sivra::canonicalizer::builtin_rules::mixed_constant_aggregation)
            .has_value());

  CHECK(canonical_expression(fixture, root, configuration) == "multiply(0, 1, x)");
}

TEST_CASE(
  "algebraic constants require matching value types"
) {
  auto operation = sivra::test_support::test_operation(
    "typed_identity",
    {
      .identity =
        sivra::ir::operation_constant{
          sivra::ir::f32_constant::from_value(0.0F),
        },
    },
    {
      .arity =
        {
          .minimum = 2,
          .maximum = std::nullopt,
        },
      .operand_types = sivra::ir::operand_type_constraint::any,
    }
  );
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto custom = fixture.operations.custom.front();
  const auto root = fixture.apply(custom, {fixture.symbol("x"), fixture.i32(0)});

  CHECK(canonical_expression(fixture, root) == "typed_identity(x, 0)");
}

TEST_CASE(
  "custom non-zero annihilators collapse matching operations"
) {
  auto operation = sivra::test_support::test_operation(
    "select_two",
    {
      .annihilator =
        sivra::ir::operation_constant{
          sivra::ir::f32_constant::from_value(2.0F),
        },
    },
    {
      .arity =
        {
          .minimum = 2,
          .maximum = std::nullopt,
        },
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto root =
    fixture.apply(fixture.operations.custom.front(), {fixture.symbol("x"), fixture.f32(2.0F)});

  CHECK(canonical_expression(fixture, root) == "2");
}

TEST_CASE(
  "constant folding evaluates built-in scalar operations"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto add =
    fixture.apply(fixture.operations.builtins.add, {fixture.f32(2.0F), fixture.f32(3.0F)});
  const auto subtract = fixture.apply(
    fixture.operations.builtins.subtract,
    {fixture.i32(9), fixture.i32(4)},
    sivra::ir::value_type::i32()
  );

  CHECK(canonical_expression(fixture, add) == "5");
  CHECK(canonical_expression(fixture, subtract) == "5");
}

TEST_CASE(
  "same-operand subtraction simplifies to zero"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto root = fixture.apply(fixture.operations.builtins.subtract, {x, x});

  CHECK(canonical_expression(fixture, root) == "0");
}

TEST_CASE(
  "same-operand simplification requires the exact built-in operation key"
) {
  sivra::ir::operation_registration operation{
    .key = sivra::ir::operation_key("subtract", 2),
    .name = "subtract_v2",
    .signature =
      {
        .arity = {.minimum = 2, .maximum = 2},
        .operand_types = sivra::ir::operand_type_constraint::same_as_result,
      },
    .attribute_schema = {},
  };
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto x = fixture.symbol("x");
  const auto root = fixture.apply(fixture.operations.custom.front(), {x, x});

  CHECK(canonical_expression(fixture, root) == "subtract_v2(x, x)");
}

TEST_CASE(
  "coefficient collection combines repeated additive terms"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto twice = fixture.apply(fixture.operations.builtins.multiply, {fixture.f32(2.0F), x});
  const auto thrice = fixture.apply(fixture.operations.builtins.multiply, {fixture.f32(3.0F), x});
  const auto root = fixture.apply(fixture.operations.builtins.add, {twice, thrice});

  CHECK(canonical_expression(fixture, root) == "multiply(5, x)");
}

TEST_CASE(
  "mixed constant aggregation combines constants inside larger expressions"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto y = fixture.symbol("y");
  const auto add =
    fixture.apply(fixture.operations.builtins.add, {fixture.f32(2.0F), x, fixture.f32(3.0F), y});
  const auto multiply =
    fixture.apply(fixture.operations.builtins.multiply, {x, fixture.f32(2.0F), fixture.f32(3.0F)});
  const auto maximum =
    fixture.apply(fixture.operations.builtins.maximum, {x, fixture.f32(3.0F), fixture.f32(5.0F)});
  const auto minimum =
    fixture.apply(fixture.operations.builtins.minimum, {x, fixture.f32(3.0F), fixture.f32(5.0F)});

  CHECK(canonical_expression(fixture, add) == "add(5, x, y)");
  CHECK(canonical_expression(fixture, multiply) == "multiply(6, x)");
  CHECK(canonical_expression(fixture, maximum) == "maximum(5, x)");
  CHECK(canonical_expression(fixture, minimum) == "minimum(3, x)");
}

TEST_CASE(
  "subtraction normalizes to addition with a negative coefficient"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto y = fixture.symbol("y");
  const auto general = fixture.apply(fixture.operations.builtins.subtract, {x, y});
  const auto constant = fixture.apply(fixture.operations.builtins.subtract, {x, fixture.f32(3.0F)});
  const auto negated = fixture.apply(fixture.operations.builtins.subtract, {fixture.f32(0.0F), x});

  CHECK(canonical_expression(fixture, general) == "add(x, multiply(-1, y))");
  CHECK(canonical_expression(fixture, constant) == "add(-3, x)");
  CHECK(canonical_expression(fixture, negated) == "multiply(-1, x)");
}

TEST_CASE(
  "coefficient collection cancels and combines negative terms"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto negative =
    fixture.apply(fixture.operations.builtins.multiply, {fixture.f32(-1.0F), x});
  const auto twice = fixture.apply(fixture.operations.builtins.multiply, {fixture.f32(2.0F), x});
  const auto cancelled = fixture.apply(fixture.operations.builtins.add, {x, negative});
  const auto reduced = fixture.apply(fixture.operations.builtins.add, {twice, negative});

  CHECK(canonical_expression(fixture, cancelled) == "0");
  CHECK(canonical_expression(fixture, reduced) == "x");
}

TEST_CASE(
  "division and reciprocal rules simplify localized inverse expressions"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto a = fixture.symbol("a");
  const auto b = fixture.symbol("b");
  const auto c = fixture.symbol("c");
  const auto divide_by_one =
    fixture.apply(fixture.operations.builtins.divide, {x, fixture.f32(1.0F)});
  const auto zero_divided =
    fixture.apply(fixture.operations.builtins.divide, {fixture.f32(0.0F), x});
  const auto zero_by_zero =
    fixture.apply(fixture.operations.builtins.divide, {fixture.f32(0.0F), fixture.f32(0.0F)});
  const auto self_divided = fixture.apply(fixture.operations.builtins.divide, {x, x});
  const auto divide_by_constant =
    fixture.apply(fixture.operations.builtins.divide, {x, fixture.f32(4.0F)});
  const auto product = fixture.apply(fixture.operations.builtins.multiply, {a, b, c});
  const auto cancelled_factor = fixture.apply(fixture.operations.builtins.divide, {product, b});
  const auto reciprocal = fixture.apply(fixture.operations.builtins.reciprocal, {x});
  const auto double_reciprocal =
    fixture.apply(fixture.operations.builtins.reciprocal, {reciprocal});
  const auto inverse_product = fixture.apply(fixture.operations.builtins.multiply, {x, reciprocal});

  CHECK(canonical_expression(fixture, divide_by_one) == "x");
  CHECK(canonical_expression(fixture, zero_divided) == "divide(0, x)");
  CHECK(canonical_expression(fixture, zero_by_zero) == "nan");
  CHECK(canonical_expression(fixture, self_divided) == "divide(x, x)");
  CHECK(canonical_expression(fixture, divide_by_constant) == "multiply(0.25, x)");
  CHECK(canonical_expression(fixture, cancelled_factor) == "divide(multiply(a, b, c), b)");
  CHECK(canonical_expression(fixture, double_reciprocal) == "x");
  CHECK(canonical_expression(fixture, inverse_product) != "1");
}

TEST_CASE(
  "bitwise rules apply identities cancellation and and-not direction"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x", sivra::ir::value_type::i32());
  const auto y = fixture.symbol("y", sivra::ir::value_type::i32());
  const auto zero = fixture.i32(0);
  const auto all_ones = fixture.i32(-1);
  const auto apply =
    [&](sivra::ir::operation_id operation, std::initializer_list<sivra::ir::node_id> operands) {
      return fixture.apply(operation, operands, sivra::ir::value_type::i32());
    };

  CHECK(canonical_expression(fixture, apply(fixture.operations.builtins.bit_and, {x, x})) == "x");
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_and, {x, zero})) == "0"
  );
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_and, {x, all_ones})) == "x"
  );
  CHECK(canonical_expression(fixture, apply(fixture.operations.builtins.bit_or, {x, zero})) == "x");
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_or, {x, all_ones})) == "-1"
  );
  CHECK(canonical_expression(fixture, apply(fixture.operations.builtins.bit_xor, {x, x})) == "0");
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_xor, {x, x, y})) == "y"
  );
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_and_not, {zero, x})) == "x"
  );
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_and_not, {all_ones, x})) ==
    "0"
  );
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_and_not, {x, zero})) == "0"
  );
  CHECK(
    canonical_expression(fixture, apply(fixture.operations.builtins.bit_and_not, {x, x})) == "0"
  );
}

TEST_CASE(
  "minimum and maximum flatten order deduplicate and fold constants"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto a = fixture.symbol("a");
  const auto b = fixture.symbol("b");
  const auto c = fixture.symbol("c");
  const auto nested = fixture.apply(fixture.operations.builtins.maximum, {c, a});
  const auto maximum = fixture.apply(fixture.operations.builtins.maximum, {nested, b, a});
  const auto minimum = fixture.apply(
    fixture.operations.builtins.minimum,
    {fixture.symbol("z"), fixture.f32(8.0F), fixture.f32(2.0F), fixture.f32(5.0F)}
  );
  const auto positive_infinity = fixture.f32(std::numeric_limits<float>::infinity());
  const auto negative_infinity = fixture.f32(-std::numeric_limits<float>::infinity());
  const auto maximum_identity =
    fixture.apply(fixture.operations.builtins.maximum, {a, negative_infinity});
  const auto maximum_annihilator =
    fixture.apply(fixture.operations.builtins.maximum, {a, positive_infinity});
  const auto minimum_identity =
    fixture.apply(fixture.operations.builtins.minimum, {a, positive_infinity});
  const auto minimum_annihilator =
    fixture.apply(fixture.operations.builtins.minimum, {a, negative_infinity});

  CHECK(canonical_expression(fixture, maximum) == "maximum(a, b, c)");
  CHECK(canonical_expression(fixture, minimum) == "minimum(2, z)");
  CHECK(canonical_expression(fixture, maximum_identity) == "a");
  CHECK(canonical_expression(fixture, maximum_annihilator) == "inf");
  CHECK(canonical_expression(fixture, minimum_identity) == "a");
  CHECK(canonical_expression(fixture, minimum_annihilator) == "-inf");
}

TEST_CASE(
  "square sqrt and copy rules normalize to stable low-level forms"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto x = fixture.symbol("x");
  const auto product = fixture.apply(fixture.operations.builtins.multiply, {x, x});
  const auto square = fixture.apply(fixture.operations.builtins.square, {x});
  const auto sqrt = fixture.apply(fixture.operations.builtins.sqrt, {x});
  const auto sqrt_square = fixture.apply(fixture.operations.builtins.sqrt, {square});
  const auto square_sqrt = fixture.apply(fixture.operations.builtins.square, {sqrt});
  const auto copy = fixture.apply(fixture.operations.builtins.copy, {x});

  CHECK(canonical_expression(fixture, product) == "square(x)");
  CHECK(canonical_expression(fixture, sqrt_square) == "sqrt(square(x))");
  CHECK(canonical_expression(fixture, square_sqrt) == "square(sqrt(x))");
  CHECK(canonical_expression(fixture, copy) == "x");
}
