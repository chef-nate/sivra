#include <sivra/ir/ir.hpp>

#include <array>
#include <utility>

int main() {
  sivra::ir::operation_catalogue_builder catalogue_builder;
  const auto builtins = sivra::ir::register_builtin_operations(catalogue_builder);
  if (!builtins.has_value()) {
    return 1;
  }

  const auto catalogue = std::move(catalogue_builder).freeze();
  if (!catalogue.has_value()) {
    return 2;
  }

  sivra::ir::expression_graph graph(*catalogue);
  sivra::ir::graph_builder graph_builder(graph);
  const auto lhs = graph_builder.make_symbol("lhs", sivra::ir::value_type::f32());
  const auto rhs = graph_builder.make_symbol("rhs", sivra::ir::value_type::f32());
  if (!lhs.has_value() || !rhs.has_value()) {
    return 3;
  }

  const std::array operands{*lhs, *rhs};
  const auto root = graph_builder.apply(builtins->add, operands, sivra::ir::value_type::f32());
  if (!root.has_value() || !graph.validate().has_value()) {
    return 4;
  }

  return graph.at(*root).get_if_operation() == nullptr ? 5 : 0;
}
