#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

#include <stdexcept>

TEST_CASE(
  "expression_graph reports its construction context"
) {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph(context);

  CHECK(&graph.context() == &context);
}

TEST_CASE(
  "expression_graph accepts operations and types from its context"
) {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph(context);
  const auto operation = context.operations().register_operation("operation");
  const auto& result_type = context.types().scalar(sivra::ir::scalar_type::f32);

  const auto node = graph.add_node(operation, result_type, {});

  CHECK(graph.at(node).operation() == operation);
  CHECK(&graph.at(node).result_type() == &result_type);
}

TEST_CASE(
  "expression_graph rejects an unknown operation identifier"
) {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph(context);
  const auto& result_type = context.types().scalar(sivra::ir::scalar_type::f32);

  CHECK_THROWS_AS(graph.add_node(sivra::ir::operation_id(0), result_type, {}), std::out_of_range);
}

TEST_CASE(
  "expression_graph rejects a result type from another context"
) {
  sivra::ir::ir_context context;
  sivra::ir::ir_context foreign_context;
  sivra::ir::expression_graph graph(context);
  const auto operation = context.operations().register_operation("operation");
  const auto& foreign_type = foreign_context.types().scalar(sivra::ir::scalar_type::f32);

  CHECK_THROWS_AS(graph.add_node(operation, foreign_type, {}), std::invalid_argument);
}

TEST_CASE(
  "expression_graph rejects a constant from another context"
) {
  sivra::ir::ir_context context;
  sivra::ir::ir_context foreign_context;
  sivra::ir::expression_graph graph(context);
  const auto constant_operation = context.operations().register_operation("constant");
  const auto& foreign_type = foreign_context.types().scalar(sivra::ir::scalar_type::f32);
  const auto foreign_value =
    sivra::ir::constant_value::scalar(foreign_type, sivra::ir::f32_constant::from_value(1.0F));

  CHECK_THROWS_AS(graph.add_constant(constant_operation, foreign_value), std::invalid_argument);
}
