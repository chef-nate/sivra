#include "../../support/graph_builder_fixture.hpp"

#include <sivra/ir/expression_node.hpp>

#include <doctest/doctest.h>

#include <array>
#include <stdexcept>

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
  CHECK(fixture.graph.at(symbol).get_if_symbol()->name == "x");
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
  CHECK_THROWS_AS(rhs.graph.at(lhs_node), std::invalid_argument);
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
  "external value identifiers survive rebuilding against the same catalogue"
) {
  sivra::test_support::graph_builder_fixture source;
  const auto external = sivra::test_support::require_value(
    source.builder.make_external_value(sivra::ir::value_type::f32())
  );
  const auto external_id = source.graph.at(external).get_if_external_value()->value;

  sivra::ir::expression_graph rebuilt(source.graph.shared_catalogue());
  sivra::ir::graph_builder builder(rebuilt);
  const auto copied = sivra::test_support::require_value(
    builder.make_external_value(external_id, sivra::ir::value_type::f32())
  );

  CHECK(rebuilt.at(copied).get_if_external_value()->value == external_id);
}

TEST_CASE(
  "external value identifiers reject a different catalogue scope"
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
