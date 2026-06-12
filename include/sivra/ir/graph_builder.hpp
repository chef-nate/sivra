#pragma once

#include "expression_graph.hpp"

#include <sivra/core/result.hpp>

#include <span>
#include <string>

namespace sivra::ir {

class graph_builder {
public:
  explicit graph_builder(
    expression_graph& graph
  );

  [[nodiscard]] core::result_t<node_id> make_constant(
    constant_value value
  ) const;

  [[nodiscard]] core::result_t<node_id> make_symbol(
    std::string name,
    value_type result_type
  ) const;

  [[nodiscard]] core::result_t<node_id> make_external_value(
    value_type result_type
  ) const;

  [[nodiscard]] core::result_t<node_id> make_external_value(
    external_value_id value,
    value_type result_type
  ) const;

  [[nodiscard]] core::result_t<node_id> make_unknown(
    std::string reason,
    value_type result_type
  ) const;

  [[nodiscard]] core::result_t<node_id> apply(
    operation_id operation,
    std::span<const node_id> operands,
    value_type result_type
  ) const;

  [[nodiscard]] core::result_t<node_id> apply(
    operation_id operation,
    std::span<const node_id> operands,
    operation_attributes attributes,
    value_type result_type
  ) const;

  [[nodiscard]] core::result_t<node_id> make_merge(
    std::span<const node_id> incoming,
    value_type result_type
  ) const;

private:
  [[nodiscard]] core::result_t<void> validate_operands(
    std::span<const node_id> operands
  ) const;

  expression_graph* m_graph;
};

} // namespace sivra::ir
