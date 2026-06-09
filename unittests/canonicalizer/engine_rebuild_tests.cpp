#include <sivra/canonicalizer/engine.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

#include <array>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace {

struct graph_fixture {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph;
  sivra::ir::operation_id symbol;
  sivra::ir::operation_id add;

  graph_fixture()
      : graph(context),
        symbol(context.operations().register_operation("symbol")),
        add(context.operations().register_operation("add")) {}

  const sivra::ir::scalar_type_def& f32() {
    return context.types().scalar(sivra::ir::scalar_type::f32);
  }

  sivra::ir::node_id add_symbol(
    std::string name
  ) {
    return graph.add_node(symbol, f32(), {}, sivra::ir::symbol_ref{.name = std::move(name)});
  }

  sivra::ir::node_id add_add(
    std::vector<sivra::ir::node_id> children
  ) {
    return graph.add_node(add, f32(), std::move(children));
  }
};

const sivra::ir::symbol_ref& symbol_value(
  const sivra::ir::expression_node& node
) {
  REQUIRE(node.leaf_value().has_value());
  const auto* value = std::get_if<sivra::ir::symbol_ref>(&*node.leaf_value());
  REQUIRE(value != nullptr);
  return *value;
}

} // namespace

TEST_CASE(
  "canonicalizer engine rebuilds reachable roots in requested order"
) {
  graph_fixture fixture;
  const auto lhs = fixture.add_symbol("lhs");
  const auto rhs = fixture.add_symbol("rhs");
  const auto sum = fixture.add_add({lhs, rhs});
  // Keep an unreachable node in the source graph so this verifies that only the
  // slice reachable from the requested roots is copied.
  static_cast<void>(fixture.add_symbol("unreachable"));

  const sivra::canonicalizer::engine engine;
  // Request both the parent expression and one child. The child should be copied
  // once and then reused by both root entries.
  const std::array roots{sum, lhs};
  const auto result =
    engine.canonicalize(fixture.graph, std::span<const sivra::ir::node_id>(roots));

  REQUIRE(result.roots.size() == 2);
  CHECK(result.graph.size() == 3);

  const auto& rebuilt_sum = result.graph.at(result.roots[0]);
  REQUIRE(rebuilt_sum.children().size() == 2);
  CHECK(rebuilt_sum.operation() == fixture.add);
  CHECK(result.roots[1] == rebuilt_sum.children()[0]);

  const auto& rebuilt_lhs = result.graph.at(result.roots[1]);
  CHECK(rebuilt_lhs.operation() == fixture.symbol);
  CHECK(symbol_value(rebuilt_lhs).name == "lhs");

  const auto& rebuilt_rhs = result.graph.at(rebuilt_sum.children()[1]);
  CHECK(rebuilt_rhs.operation() == fixture.symbol);
  CHECK(symbol_value(rebuilt_rhs).name == "rhs");
}

TEST_CASE(
  "canonicalizer engine preserves shared source nodes and result type references"
) {
  graph_fixture fixture;
  // Build two parents over the same source child so the rebuilt graph must
  // preserve DAG sharing rather than duplicating the child node.
  const auto value = fixture.add_symbol("x");
  const auto left = fixture.add_add({value});
  const auto right = fixture.add_add({value});
  const auto root = fixture.add_add({left, right});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  const auto& rebuilt_root = result.graph.at(result.root);
  REQUIRE(rebuilt_root.children().size() == 2);

  const auto& rebuilt_left = result.graph.at(rebuilt_root.children()[0]);
  const auto& rebuilt_right = result.graph.at(rebuilt_root.children()[1]);
  REQUIRE(rebuilt_left.children().size() == 1);
  REQUIRE(rebuilt_right.children().size() == 1);

  CHECK(rebuilt_left.children()[0] == rebuilt_right.children()[0]);
  // Rebuilt nodes keep references to type_context-owned type objects; the
  // canonicalizer does not clone or own type definitions.
  CHECK(&rebuilt_root.result_type() == &fixture.f32());
  CHECK(&rebuilt_left.result_type() == &fixture.f32());
  CHECK(&rebuilt_right.result_type() == &fixture.f32());
  CHECK(&result.graph.at(rebuilt_left.children()[0]).result_type() == &fixture.f32());
}
