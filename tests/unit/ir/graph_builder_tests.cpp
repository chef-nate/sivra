#include "../../support/graph_builder_fixture.hpp"

#include <sivra/ir/expression_node.hpp>
#include <sivra/ir/structural.hpp>

#include <doctest/doctest.h>

#include <array>
#include <stdexcept>
#include <type_traits>

static_assert(
  !std::is_constructible_v<
    sivra::ir::expression_node,
    sivra::ir::node_id,
    sivra::ir::value_type,
    sivra::ir::expression_payload_t
  >
);

TEST_CASE(
  "graph builder creates typed variant nodes"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto constant = fixture.f32(4.0F);
  const auto symbol = fixture.symbol("x");
  const auto external = sivra::test_support::require_value(
    fixture.builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto unknown = sivra::test_support::require_value(
    fixture.builder.make_unknown("not recovered", sivra::ir::value_type::f32())
  );
  const auto sum = fixture.apply(fixture.operations.builtins.add, {symbol, external});

  CHECK(fixture.graph.at(constant).get_if_constant() != nullptr);
  CHECK(fixture.graph.symbol_name(fixture.graph.at(symbol).get_if_symbol()->symbol) == "x");
  CHECK(fixture.graph.at(external).get_if_external_value()->value.index() == 0);
  CHECK(fixture.graph.at(unknown).get_if_unknown()->reason == "not recovered");
  REQUIRE(fixture.graph.at(sum).get_if_operation() != nullptr);
  CHECK(fixture.graph.at(sum).operands().size() == 2);
}

TEST_CASE(
  "graph builder assigns owner-scoped stable node identifiers"
) {
  sivra::test_support::graph_builder_fixture lhs;
  sivra::test_support::graph_builder_fixture rhs;
  const auto lhs_node = lhs.symbol("lhs");
  const auto rhs_node = rhs.symbol("rhs");

  CHECK(lhs_node.index() == rhs_node.index());
  CHECK(lhs_node != rhs_node);
  CHECK(lhs.graph.contains(lhs_node));
  CHECK(!rhs.graph.contains(lhs_node));
  CHECK_THROWS_AS(static_cast<void>(rhs.graph.at(lhs_node)), std::invalid_argument);
}

TEST_CASE(
  "graph builder rejects foreign operations without mutating the graph"
) {
  sivra::test_support::graph_builder_fixture fixture;
  sivra::test_support::graph_builder_fixture foreign;
  const auto lhs = fixture.symbol("lhs");
  const auto rhs = fixture.symbol("rhs");
  const std::array operands{lhs, rhs};
  const auto size_before = fixture.graph.size();

  const auto result =
    fixture.builder.apply(foreign.operations.builtins.add, operands, sivra::ir::value_type::f32());

  REQUIRE(!result.has_value());
  CHECK(result.error().front().code == "ir.graph.foreign_operation");
  CHECK(fixture.graph.size() == size_before);
}

TEST_CASE(
  "graph builder rejects foreign and future operands atomically"
) {
  sivra::test_support::graph_builder_fixture fixture;
  sivra::test_support::graph_builder_fixture foreign;
  const auto existing = fixture.symbol("existing");
  const auto foreign_node = foreign.symbol("foreign");
  const auto future = sivra::ir::node_id::unsafe_from_index(99, fixture.graph.owner());
  const auto size_before = fixture.graph.size();

  const std::array foreign_operands{existing, foreign_node};
  const auto foreign_result = fixture.builder.apply(
    fixture.operations.builtins.add, foreign_operands, sivra::ir::value_type::f32()
  );
  const std::array future_operands{existing, future};
  const auto future_result = fixture.builder.apply(
    fixture.operations.builtins.add, future_operands, sivra::ir::value_type::f32()
  );

  CHECK(!foreign_result.has_value());
  CHECK(!future_result.has_value());
  CHECK(fixture.graph.size() == size_before);
}

TEST_CASE(
  "graph builder enforces operation arity"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto lhs = fixture.symbol("lhs");
  const std::array too_few{lhs};

  const auto add =
    fixture.builder.apply(fixture.operations.builtins.add, too_few, sivra::ir::value_type::f32());
  CHECK(!add.has_value());
  CHECK(add.error().front().code == "ir.graph.invalid_arity");

  const auto rhs = fixture.symbol("rhs");
  const auto extra = fixture.symbol("extra");
  const std::array too_many{lhs, rhs, extra};
  const auto subtract = fixture.builder.apply(
    fixture.operations.builtins.subtract, too_many, sivra::ir::value_type::f32()
  );
  CHECK(!subtract.has_value());
  CHECK(subtract.error().front().code == "ir.graph.invalid_arity");
}

TEST_CASE(
  "graph builder enforces same-type operation signatures"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto float_value = fixture.symbol("float");
  const auto integer_value = fixture.symbol("integer", sivra::ir::value_type::i32());
  const std::array operands{float_value, integer_value};

  const auto result =
    fixture.builder.apply(fixture.operations.builtins.add, operands, sivra::ir::value_type::f32());

  REQUIRE(!result.has_value());
  CHECK(result.error().front().code == "ir.graph.type_mismatch");
}

TEST_CASE(
  "external value identifiers survive rebuilding against the same external value scope"
) {
  sivra::test_support::graph_builder_fixture source;
  const auto external = sivra::test_support::require_value(
    source.builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto external_id = source.graph.at(external).get_if_external_value()->value;

  sivra::ir::expression_graph rebuilt(
    source.graph.shared_catalogue(), source.graph.external_value_owner()
  );
  sivra::ir::graph_builder builder(rebuilt);
  const auto copied = sivra::test_support::require_value(
    builder.make_external_value(external_id, sivra::ir::value_type::f32())
  );

  CHECK(rebuilt.at(copied).get_if_external_value()->value == external_id);
}

TEST_CASE(
  "auto-allocated external value identifiers use distinct graph scopes"
) {
  const auto operations = sivra::test_support::make_test_catalogue();
  sivra::ir::expression_graph lhs_graph(operations.catalogue);
  sivra::ir::expression_graph rhs_graph(operations.catalogue);
  sivra::ir::graph_builder lhs_builder(lhs_graph);
  sivra::ir::graph_builder rhs_builder(rhs_graph);

  const auto lhs = sivra::test_support::require_value(
    lhs_builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto rhs = sivra::test_support::require_value(
    rhs_builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto lhs_external = lhs_graph.at(lhs).get_if_external_value()->value;
  const auto rhs_external = rhs_graph.at(rhs).get_if_external_value()->value;
  sivra::ir::structural_context structural;

  CHECK(lhs_external.index() == rhs_external.index());
  CHECK(lhs_external.owner() != rhs_external.owner());
  CHECK(!structural.equal(lhs_graph, lhs, rhs_graph, rhs));
}

TEST_CASE(
  "external value identifiers reject a different external value scope"
) {
  sivra::test_support::graph_builder_fixture source;
  sivra::test_support::graph_builder_fixture target;
  const auto external = sivra::test_support::require_value(
    source.builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto external_id = source.graph.at(external).get_if_external_value()->value;

  const auto result = target.builder.make_external_value(external_id, sivra::ir::value_type::f32());
  REQUIRE(!result.has_value());
  CHECK(result.error().front().code == "ir.graph.foreign_external_value");
}

TEST_CASE(
  "graph builder validates operation attributes before insertion"
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
  auto registration = sivra::test_support::test_operation(
    "extract",
    {},
    {
      .arity = {.minimum = 1, .maximum = 1},
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  registration.attribute_schema = schema;
  sivra::test_support::graph_builder_fixture fixture({std::move(registration)});
  const auto input = fixture.symbol("input");
  const std::array operands{input};
  const auto size_before = fixture.graph.size();

  const auto missing = fixture.builder.apply(
    fixture.operations.custom.front(),
    operands,
    sivra::ir::operation_attributes{},
    sivra::ir::value_type::f32()
  );
  REQUIRE(!missing.has_value());
  CHECK(missing.error().front().code == "ir.attribute.required");
  CHECK(fixture.graph.size() == size_before);

  const auto invalid_attributes = sivra::test_support::require_value(
    sivra::ir::operation_attributes::create(
      std::array{
        sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{4}},
      }
    )
  );
  const auto invalid = fixture.builder.apply(
    fixture.operations.custom.front(), operands, invalid_attributes, sivra::ir::value_type::f32()
  );
  REQUIRE(!invalid.has_value());
  CHECK(invalid.error().front().code == "ir.attribute.range");
  CHECK(fixture.graph.size() == size_before);

  const auto valid_attributes = sivra::test_support::require_value(
    sivra::ir::operation_attributes::create(
      std::array{
        sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{2}},
      }
    )
  );
  const auto valid = fixture.builder.apply(
    fixture.operations.custom.front(), operands, valid_attributes, sivra::ir::value_type::f32()
  );
  REQUIRE(valid.has_value());
  const auto* application = fixture.graph.at(*valid).get_if_operation();
  REQUIRE(application != nullptr);
  REQUIRE(application->attributes.find("lane") != nullptr);
  CHECK(std::get<std::int64_t>(*application->attributes.find("lane")) == 2);
}

TEST_CASE(
  "graph builder creates typed merges and rejects malformed merges atomically"
) {
  sivra::test_support::graph_builder_fixture fixture;
  sivra::test_support::graph_builder_fixture foreign;
  const auto lhs = fixture.symbol("lhs");
  const auto rhs = fixture.symbol("rhs");
  const std::array incoming{lhs, rhs};

  const auto merge = fixture.builder.make_merge(incoming, sivra::ir::value_type::f32());
  REQUIRE(merge.has_value());
  REQUIRE(fixture.graph.at(*merge).get_if_merge() != nullptr);
  CHECK(fixture.graph.at(*merge).operands().size() == 2);

  const auto size_before = fixture.graph.size();
  const std::array too_few{lhs};
  CHECK(!fixture.builder.make_merge(too_few, sivra::ir::value_type::f32()).has_value());
  CHECK(fixture.graph.size() == size_before);

  const auto integer = fixture.symbol("integer", sivra::ir::value_type::i32());
  const std::array wrong_types{lhs, integer};
  CHECK(!fixture.builder.make_merge(wrong_types, sivra::ir::value_type::f32()).has_value());

  const std::array wrong_owner{lhs, foreign.symbol("foreign")};
  CHECK(!fixture.builder.make_merge(wrong_owner, sivra::ir::value_type::f32()).has_value());
  CHECK(fixture.graph.size() == size_before + 1);
}
