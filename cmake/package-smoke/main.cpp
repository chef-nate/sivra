#include <sivra/canonicalizer/canonicalizer.hpp>
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
  const auto zero_value = sivra::ir::constant_value::scalar(
    sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(0.0F)
  );
  if (!lhs.has_value() || !zero_value.has_value()) {
    return 3;
  }
  const auto zero = graph_builder.make_constant(*zero_value);
  if (!zero.has_value()) {
    return 4;
  }

  const std::array operands{*lhs, *zero};
  const auto root = graph_builder.apply(builtins->add, operands, sivra::ir::value_type::f32());
  if (!root.has_value() || !graph.validate().has_value()) {
    return 5;
  }

  const sivra::canonicalizer::engine canonicalizer;
  const auto result = canonicalizer.canonicalize(graph, *root);
  if (result.status != sivra::core::analysis_status::complete || !result.root.has_value()) {
    return 6;
  }

  return result.graph.at(*result.root).get_if_symbol() == nullptr ? 7 : 0;
}
