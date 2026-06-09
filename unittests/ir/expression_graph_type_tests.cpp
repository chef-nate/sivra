#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/operation_registry.hpp>
#include <sivra/ir/scalar_type.hpp>
#include <sivra/ir/type.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "expression_graph nodes reference result types"
) {
  sivra::ir::type_context types;
  sivra::ir::operation_registry operations;
  sivra::ir::expression_graph graph;

  const auto operation = operations.register_operation("load");
  const auto& f32 = types.scalar(sivra::ir::scalar_type::f32);
  const auto& vector = types.vector(f32, 4);

  const auto node = graph.add_node(operation, vector, {});
  const auto& stored = graph.at(node);

  CHECK(&stored.result_type() == &vector);
  CHECK(stored.result_type().kind() == sivra::ir::type_kind::vector);
}
