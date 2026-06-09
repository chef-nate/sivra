#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace {

struct graph_fixture {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph;
  sivra::ir::operation_id operation;
  const sivra::ir::scalar_type_def& result_type;

  graph_fixture()
      : graph(context),
        operation(context.operations().register_operation("operation")),
        result_type(context.types().scalar(sivra::ir::scalar_type::f32)) {}
};

} // namespace

TEST_CASE(
  "expression_graph accepts existing child identifiers"
) {
  graph_fixture fixture;
  const auto lhs = fixture.graph.add_node(fixture.operation, fixture.result_type, {});
  const auto rhs = fixture.graph.add_node(fixture.operation, fixture.result_type, {});
  const auto parent =
    fixture.graph.add_node(fixture.operation, fixture.result_type, {rhs, lhs, rhs});

  const auto children = fixture.graph.at(parent).children();
  REQUIRE(children.size() == 3);
  CHECK(children[0] == rhs);
  CHECK(children[1] == lhs);
  CHECK(children[2] == rhs);
}

TEST_CASE(
  "expression_graph rejects a self child identifier"
) {
  graph_fixture fixture;
  const auto size_before = fixture.graph.size();

  CHECK_THROWS_AS(
    fixture.graph.add_node(
      fixture.operation,
      fixture.result_type,
      {sivra::ir::node_id(static_cast<std::uint32_t>(size_before))}
    ),
    std::invalid_argument
  );
  CHECK(fixture.graph.size() == size_before);
}

TEST_CASE(
  "expression_graph rejects a future child identifier"
) {
  graph_fixture fixture;
  const auto existing = fixture.graph.add_node(fixture.operation, fixture.result_type, {});
  const auto size_before = fixture.graph.size();

  CHECK_THROWS_AS(
    fixture.graph.add_node(fixture.operation, fixture.result_type, {sivra::ir::node_id(2)}),
    std::invalid_argument
  );
  CHECK(fixture.graph.size() == size_before);
  CHECK(fixture.graph.at(existing).id() == existing);
}

TEST_CASE(
  "expression_graph rejects an out-of-range child identifier"
) {
  graph_fixture fixture;
  const auto size_before = fixture.graph.size();

  CHECK_THROWS_AS(
    fixture.graph.add_node(
      fixture.operation,
      fixture.result_type,
      {sivra::ir::node_id(std::numeric_limits<std::uint32_t>::max())}
    ),
    std::invalid_argument
  );
  CHECK(fixture.graph.size() == size_before);
}

TEST_CASE(
  "expression_graph rejects mixed valid and invalid children atomically"
) {
  graph_fixture fixture;
  const auto existing = fixture.graph.add_node(fixture.operation, fixture.result_type, {});
  const auto size_before = fixture.graph.size();

  CHECK_THROWS_AS(
    fixture.graph.add_node(
      fixture.operation, fixture.result_type, {existing, sivra::ir::node_id(1)}
    ),
    std::invalid_argument
  );
  CHECK(fixture.graph.size() == size_before);
  CHECK(fixture.graph.at(existing).children().empty());
}
