#include <sivra/ir/validation.hpp>

#include <sivra/ir/constant.hpp>

#include <string>

namespace {

void add_error(
  sivra::core::diagnostic_bundle_t& diagnostics,
  std::string code,
  std::string message
) {
  diagnostics.push_back(
    {
      .code = std::move(code),
      .severity = sivra::core::diagnostic_severity::error,
      .message = std::move(message),
    }
  );
}

bool type_is_valid(
  const sivra::ir::value_type& type
) {
  switch (type.kind()) {
  case sivra::ir::value_type_kind::unknown:
    return type.category() == sivra::ir::scalar_category::unknown && type.lane_count() == 0;
  case sivra::ir::value_type_kind::scalar:
    return type.category() != sivra::ir::scalar_category::unknown &&
           type.element_bit_width() != 0 && type.lane_count() == 1;
  case sivra::ir::value_type_kind::vector:
    return type.category() != sivra::ir::scalar_category::unknown &&
           type.element_bit_width() != 0 && type.lane_count() != 0;
  }
  return false;
}

} // namespace

namespace sivra::ir {

core::result_t<void> validate_graph(
  const expression_graph& graph
) {
  core::diagnostic_bundle_t diagnostics;

  for (std::size_t index = 0; index < graph.size(); ++index) {
    const auto expected_id =
      node_id::unsafe_from_index(static_cast<std::uint32_t>(index), graph.owner());
    const auto& node = graph.nodes()[index];
    if (node.id() != expected_id) {
      add_error(
        diagnostics, "ir.validation.node_id", "node identifier does not match its graph position"
      );
    }
    if (!type_is_valid(node.result_type())) {
      add_error(diagnostics, "ir.validation.value_type", "node has an invalid result type");
    }

    if (const auto* constant = node.get_if_constant()) {
      if (constant->value.result_type() != node.result_type()) {
        add_error(
          diagnostics,
          "ir.validation.constant_type",
          "constant value type does not match node result type"
        );
      }
      continue;
    }
    if (const auto* symbol = node.get_if_symbol()) {
      if (symbol->symbol.owner() != graph.owner()) {
        add_error(
          diagnostics, "ir.validation.symbol_owner", "symbol identifier belongs to another graph"
        );
      } else {
        try {
          if (graph.symbol_name(symbol->symbol).empty()) {
            add_error(diagnostics, "ir.validation.symbol", "symbol name must not be empty");
          }
        } catch (const std::out_of_range&) {
          add_error(
            diagnostics, "ir.validation.symbol", "symbol identifier is not present in the graph"
          );
        }
      }
      continue;
    }
    if (const auto* external = node.get_if_external_value()) {
      if (external->value.owner() != graph.external_value_owner()) {
        add_error(
          diagnostics,
          "ir.validation.external_owner",
          "external value identifier belongs to another external value scope"
        );
      }
      continue;
    }
    if (node.get_if_unknown() != nullptr) {
      if (node.get_if_unknown()->reason.empty()) {
        add_error(diagnostics, "ir.validation.unknown_reason", "unknown reason must not be empty");
      }
      continue;
    }

    if (const auto* merge = node.get_if_merge()) {
      if (merge->incoming.size() < 2) {
        add_error(diagnostics, "ir.validation.merge_arity", "merge node has fewer than two inputs");
      }
      for (const auto incoming : merge->incoming) {
        if (incoming.owner() != graph.owner() || incoming.index() >= index) {
          add_error(
            diagnostics,
            "ir.validation.merge_input",
            "merge input must be an earlier node in the same graph"
          );
          continue;
        }
        if (graph.at(incoming).result_type() != node.result_type()) {
          add_error(
            diagnostics,
            "ir.validation.merge_type",
            "merge input type does not match node result type"
          );
        }
      }
      continue;
    }

    const auto* application = node.get_if_operation();
    if (application == nullptr) {
      add_error(diagnostics, "ir.validation.payload", "node has an unknown payload kind");
      continue;
    }
    if (application->operation.owner() != graph.catalogue().owner()) {
      add_error(
        diagnostics,
        "ir.validation.operation_owner",
        "operation identifier belongs to another catalogue"
      );
      continue;
    }

    const auto& definition = graph.catalogue().operation(application->operation);
    std::vector<value_type> operand_types;
    operand_types.reserve(application->operands.size());
    for (const auto operand : application->operands) {
      if (operand.owner() != graph.owner() || operand.index() >= index) {
        add_error(
          diagnostics,
          "ir.validation.operand",
          "operation operand must be an earlier node in the same graph"
        );
        continue;
      }
      operand_types.push_back(graph.at(operand).result_type());
    }

    if (operand_types.size() == application->operands.size()) {
      auto validated = definition.signature().validate_application(
        node.result_type(), operand_types, application->attributes, definition.attribute_schema()
      );
      if (!validated.has_value()) {
        diagnostics.insert(
          diagnostics.end(),
          std::make_move_iterator(validated.error().begin()),
          std::make_move_iterator(validated.error().end())
        );
      }
    }
  }

  if (!diagnostics.empty()) {
    return std::unexpected(std::move(diagnostics));
  }
  return {};
}

core::result_t<void> expression_graph::validate() const {
  return validate_graph(*this);
}

} // namespace sivra::ir
