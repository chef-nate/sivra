#include "../../support/graph_builder_fixture.hpp"

#include <sivra/canonicalizer/engine.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <array>

TEST_CASE(
  "canonicalizer rebuilds requested roots in order"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto lhs = fixture.symbol("lhs");
  const auto rhs = fixture.symbol("rhs");
  const auto sum = fixture.apply(fixture.operations.builtins.add, {lhs, rhs});
  const std::array roots{rhs, sum, lhs};

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, roots);

  REQUIRE(result.roots.size() == 3);
  CHECK(
    result.graph.symbol_name(result.graph.at(result.roots[0]).get_if_symbol()->symbol) == "rhs"
  );
  CHECK(
    result.graph.symbol_name(result.graph.at(result.roots[2]).get_if_symbol()->symbol) == "lhs"
  );
  REQUIRE(result.graph.at(result.roots[1]).get_if_operation() != nullptr);
}

TEST_CASE(
  "canonicalizer preserves duplicate roots and shared children"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto shared = fixture.symbol("shared");
  const auto lhs = fixture.apply(fixture.operations.builtins.subtract, {shared, fixture.f32(1.0F)});
  const auto rhs = fixture.apply(fixture.operations.builtins.maximum, {shared, fixture.f32(2.0F)});
  const auto root = fixture.apply(fixture.operations.builtins.add, {lhs, rhs});
  const std::array roots{root, root};

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, roots);

  CHECK(result.roots[0] == result.roots[1]);
  const auto root_operands = result.graph.at(result.roots[0]).operands();
  REQUIRE(root_operands.size() == 3);
  const auto shared_operand = std::ranges::find_if(root_operands, [&](sivra::ir::node_id operand) {
    return result.graph.at(operand).get_if_symbol() != nullptr;
  });
  REQUIRE(shared_operand != root_operands.end());
  const auto maximum_operand = std::ranges::find_if(root_operands, [&](sivra::ir::node_id operand) {
    const auto* application = result.graph.at(operand).get_if_operation();
    return application != nullptr &&
           result.graph.catalogue().operation(application->operation).stable_key() ==
             sivra::ir::operation_key("maximum");
  });
  REQUIRE(maximum_operand != root_operands.end());
  CHECK(
    std::ranges::find(result.graph.at(*maximum_operand).operands(), *shared_operand) !=
    result.graph.at(*maximum_operand).operands().end()
  );
}

TEST_CASE(
  "canonicalizer output owns new nodes and shares the immutable catalogue"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto root = fixture.symbol("x");

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  CHECK(result.graph.owner() != fixture.graph.owner());
  CHECK(result.root->owner() == result.graph.owner());
  CHECK(result.graph.shared_catalogue() == fixture.graph.shared_catalogue());
  CHECK(result.graph.symbol_name(result.graph.at(*result.root).get_if_symbol()->symbol) == "x");
}

TEST_CASE(
  "canonicalizer copies every leaf variant"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto constant = fixture.f32(3.0F);
  const auto symbol = fixture.symbol("x");
  const auto external = sivra::test_support::require_value(
    fixture.builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto unknown = sivra::test_support::require_value(
    fixture.builder.make_unknown("unknown source", sivra::ir::value_type::f32())
  );
  const std::array roots{constant, symbol, external, unknown};

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, roots);

  CHECK(result.graph.at(result.roots[0]).get_if_constant() != nullptr);
  CHECK(result.graph.at(result.roots[1]).get_if_symbol() != nullptr);
  CHECK(result.graph.at(result.roots[2]).get_if_external_value() != nullptr);
  CHECK(result.graph.at(result.roots[3]).get_if_unknown() != nullptr);
}

TEST_CASE(
  "canonicalizer rejects roots owned by another graph"
) {
  sivra::test_support::graph_builder_fixture fixture;
  sivra::test_support::graph_builder_fixture foreign;
  const auto foreign_root = foreign.symbol("foreign");

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, foreign_root);

  CHECK(result.status == sivra::core::analysis_status::invalid_input);
  CHECK(!result.root.has_value());
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code == "canonicalizer.invalid_root");
}

TEST_CASE(
  "canonicalizer preserves merge nodes and operation attributes"
) {
  const auto schema = sivra::test_support::require_value(
    sivra::ir::operation_attribute_schema::create(
      std::array{
        sivra::ir::operation_attribute_field{
          .key = "lane",
          .kind = sivra::ir::operation_attribute_kind::integer,
          .required = true,
          .minimum_integer = 0,
          .maximum_integer = 3,
        },
      }
    )
  );
  auto operation = sivra::test_support::test_operation(
    "extract",
    {},
    {
      .arity = {.minimum = 1, .maximum = 1},
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  operation.attribute_schema = schema;
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const std::array incoming{fixture.symbol("lhs"), fixture.symbol("rhs")};
  const auto merge = sivra::test_support::require_value(
    fixture.builder.make_merge(incoming, sivra::ir::value_type::f32())
  );
  const auto attributes = sivra::test_support::require_value(
    sivra::ir::operation_attributes::create(
      std::array{
        sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{2}},
      }
    )
  );
  const std::array operands{merge};
  const auto root = sivra::test_support::require_value(fixture.builder.apply(
    fixture.operations.custom.front(), operands, attributes, sivra::ir::value_type::f32()
  ));

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  REQUIRE(result.root.has_value());
  const auto* application = result.graph.at(*result.root).get_if_operation();
  REQUIRE(application != nullptr);
  REQUIRE(application->attributes.find("lane") != nullptr);
  CHECK(std::get<std::int64_t>(*application->attributes.find("lane")) == 2);
  REQUIRE(application->operands.size() == 1);
  CHECK(result.graph.at(application->operands.front()).get_if_merge() != nullptr);
}
