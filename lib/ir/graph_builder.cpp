#include <sivra/ir/graph_builder.hpp>

#include <exception>
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
) const {
  const auto result_type = value.result_type();
  return m_graph->append_validated(result_type, constant_node{.value = std::move(value)});
}

core::result_t<node_id> graph_builder::make_symbol(
  std::string name,
  value_type result_type
) const {
  if (name.empty()) {
    return core::fail<node_id>("ir.graph.empty_symbol", "symbol name must not be empty");
  }
  const auto symbol = m_graph->allocate_symbol_id(std::move(name));
  return m_graph->append_validated(result_type, symbol_node{.symbol = symbol});
}

core::result_t<node_id> graph_builder::make_external_value(
  const value_type result_type
) const {
  return make_external_value(m_graph->allocate_external_value_id(), result_type);
}

core::result_t<node_id> graph_builder::make_external_value(
  const external_value_id value,
  const value_type result_type
) const {
  if (value.owner() != m_graph->catalogue().owner()) {
    return core::fail<node_id>(
      "ir.graph.foreign_external_value", "external_value_id belongs to another catalogue scope"
    );
  }
  return m_graph->append_validated(result_type, external_value_node{.value = value});
}

core::result_t<node_id> graph_builder::make_unknown(
  std::string reason,
  const value_type result_type
) const {
  if (reason.empty()) {
    return core::fail<node_id>("ir.graph.empty_unknown_reason", "unknown reason must not be empty");
  }
  return m_graph->append_validated(result_type, unknown_node{.reason = std::move(reason)});
}

core::result_t<node_id> graph_builder::apply(
  const operation_id operation,
  std::span<const node_id> operands,
  value_type result_type
) const {
  return apply(operation, operands, operation_attributes{}, std::move(result_type));
}

core::result_t<node_id> graph_builder::apply(
  const operation_id operation,
  std::span<const node_id> operands,
  operation_attributes attributes,
  value_type result_type
) const {
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

  std::vector<value_type> operand_types;
  operand_types.reserve(operands.size());
  for (const auto operand : operands) {
    operand_types.push_back(m_graph->at(operand).result_type());
  }
  auto validated_attributes = definition->attribute_schema().validate(attributes);
  if (!validated_attributes.has_value()) {
    return std::unexpected(std::move(validated_attributes.error()));
  }
  if (auto validated = definition->signature().validate_application(
        result_type, operand_types, *validated_attributes, definition->attribute_schema()
      );
      !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }

  return m_graph->append_validated(
    result_type,
    operation_application{
      .operation = operation,
      .operands = std::vector<node_id>(operands.begin(), operands.end()),
      .attributes = std::move(*validated_attributes),
    }
  );
}

core::result_t<node_id> graph_builder::make_merge(
  std::span<const node_id> incoming,
  value_type result_type
) const {
  if (incoming.size() < 2) {
    return core::fail<node_id>(
      "ir.graph.invalid_merge_arity", "merge nodes require at least two incoming values"
    );
  }
  if (auto validated = validate_operands(incoming); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }
  for (const auto operand : incoming) {
    if (m_graph->at(operand).result_type() != result_type) {
      return core::fail<node_id>(
        "ir.graph.merge_type_mismatch", "merge input type does not match result type"
      );
    }
  }
  return m_graph->append_validated(
    result_type, merge_node{.incoming = std::vector<node_id>(incoming.begin(), incoming.end())}
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
