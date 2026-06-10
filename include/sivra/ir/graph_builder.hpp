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

  core::result_t<node_id> make_constant(
    constant_value value
  );

  core::result_t<node_id> make_symbol(
    std::string name,
    value_type result_type
  );

  core::result_t<node_id> make_external_value(
    value_type result_type
  );

  core::result_t<node_id> make_external_value(
    external_value_id value,
    value_type result_type
  );

  core::result_t<node_id> make_unknown(
    std::string reason,
    value_type result_type
  );

  core::result_t<node_id> apply(
    operation_id operation,
    std::span<const node_id> operands,
    value_type result_type
  );

private:
  core::result_t<void> validate_operands(
    std::span<const node_id> operands
  ) const;

  expression_graph* m_graph;
};

} // namespace sivra::ir
