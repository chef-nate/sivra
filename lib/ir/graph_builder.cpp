#include <sivra/ir/graph_builder.hpp>

#include <exception>
#include <limits>
#include <utility>
#include <vector>

namespace sivra::ir {

graph_builder::graph_builder(
  expression_graph& graph
)
    : m_graph(&graph) {
}

core::result_t<node_id> graph_builder::make_constant(
  constant_value value
) {
  const auto result_type = value.result_type();
  return m_graph->append_validated(result_type, constant_node{.value = std::move(value)});
}

core::result_t<node_id> graph_builder::make_symbol(
  std::string name,
  value_type result_type
) {
  if (name.empty()) {
    return core::fail<node_id>("ir.graph.empty_symbol", "symbol name must not be empty");
  }
  return m_graph->append_validated(std::move(result_type), symbol_node{.name = std::move(name)});
}

core::result_t<node_id> graph_builder::make_external_value(
  value_type result_type
) {
  return make_external_value(m_graph->allocate_external_value_id(), std::move(result_type));
}

core::result_t<node_id> graph_builder::make_external_value(
  external_value_id value,
  value_type result_type
) {
  if (value.owner() != m_graph->catalogue().owner()) {
    return core::fail<node_id>(
      "ir.graph.foreign_external_value", "external_value_id belongs to another catalogue scope"
    );
  }
  return m_graph->append_validated(std::move(result_type), external_value_node{.value = value});
}

core::result_t<node_id> graph_builder::make_unknown(
  std::string reason,
  value_type result_type
) {
  return m_graph->append_validated(
    std::move(result_type), unknown_node{.reason = std::move(reason)}
  );
}

core::result_t<node_id> graph_builder::apply(
  operation_id operation,
  std::span<const node_id> operands,
  value_type result_type
) {
  if (operation.owner() != m_graph->catalogue().owner()) {
    return core::fail<node_id>(
      "ir.graph.foreign_operation", "operation_id belongs to another catalogue"
    );
  }

  const operation_def* definition = nullptr;
  try {
    definition = &m_graph->catalogue().operation(operation);
  } catch (const std::exception& error) {
    return core::fail<node_id>("ir.graph.invalid_operation", error.what());
  }

  if (auto validated = validate_operands(operands); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }

  const auto operand_count = operands.size();
  if (operand_count < definition->signature().minimum_operands ||
      (definition->signature().maximum_operands.has_value() &&
       operand_count > *definition->signature().maximum_operands)) {
    return core::fail<node_id>(
      "ir.graph.invalid_arity", "operation operand count does not match its signature"
    );
  }

  if (definition->signature().operands_match_result) {
    for (const auto operand : operands) {
      if (m_graph->at(operand).result_type() != result_type) {
        return core::fail<node_id>(
          "ir.graph.type_mismatch", "operation operand type does not match result type"
        );
      }
    }
  }

  return m_graph->append_validated(
    std::move(result_type),
    operation_application{
      .operation = operation,
      .operands = std::vector<node_id>(operands.begin(), operands.end()),
    }
  );
}

core::result_t<void> graph_builder::validate_operands(
  std::span<const node_id> operands
) const {
  for (const auto operand : operands) {
    if (!m_graph->contains(operand)) {
      return core::fail<void>(
        "ir.graph.invalid_operand", "operand must identify an existing node in this graph"
      );
    }
  }
  return {};
}

} // namespace sivra::ir
