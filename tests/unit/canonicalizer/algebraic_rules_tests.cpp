#include "../../support/expression_format.hpp"
#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/configuration.hpp>
#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/rule.hpp>

#include <doctest/doctest.h>

#include <array>
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

  CHECK(canonical_expression(fixture, root, configuration) == "multiply(x, 0, 1)");
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
