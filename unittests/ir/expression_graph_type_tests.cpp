#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "expression_graph nodes reference result types"
) {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph(context);

  const auto operation = context.operations().register_operation("load");
  const auto& f32 = context.types().scalar(sivra::ir::scalar_type::f32);
  const auto& vector = context.types().vector(f32, 4);

  const auto node = graph.add_node(operation, vector, {});
  const auto& stored = graph.at(node);

  CHECK(&stored.result_type() == &vector);
  CHECK(stored.result_type().kind() == sivra::ir::type_kind::vector);
}
