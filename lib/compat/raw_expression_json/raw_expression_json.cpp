#include "raw_expression_json.hpp"

#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/graph_builder.hpp>
#include <sivra/ir/value_type.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

template <typename T>
sivra::core::result_t<T> compatibility_failure(
  std::string message
) {
  return sivra::core::fail<T>("compat.raw_expression_json.invalid_input", std::move(message));
}

template <typename T>
sivra::core::result_t<T> propagate_ir_error(
  sivra::core::result_t<T> result
) {
  if (!result.has_value()) {
    return std::unexpected(std::move(result.error()));
  }
  return result;
}

struct raw_node {
  std::uint32_t id;
  std::string kind;
  std::string operation;
  sivra::ir::value_type result_type;
  std::string base_register;
  std::ptrdiff_t offset = 0;
  std::vector<std::uint32_t> children;
};

sivra::ir::value_type parse_value_type(
  const nlohmann::json& node
) {
  const auto& type = node.at("type");
  const auto scalar = type.at("scalar").get<std::string>();
  const auto element_bits = type.at("element_bits").get<std::uint32_t>();
  const auto element_count = type.at("lane_count").get<std::uint32_t>();

  if (scalar == "floating_point" && element_bits == 32 && element_count == 1) {
    return sivra::ir::value_type::f32();
  }

  throw std::runtime_error("unsupported raw expression node type");
}

std::vector<std::uint32_t> parse_children(
  const nlohmann::json& node
) {
  std::vector<std::uint32_t> children;
  const auto& child_values = node.at("children");
  children.reserve(child_values.size());
  for (const auto& child : child_values) {
    children.push_back(child.at("id").get<std::uint32_t>());
  }

  return children;
}

raw_node parse_node(
  const nlohmann::json& node
) {
  raw_node parsed{
    .id = node.at("id").get<std::uint32_t>(),
    .kind = node.at("kind").get<std::string>(),
    .result_type = parse_value_type(node),
    .children = parse_children(node),
  };

  if (parsed.kind == "memory_load") {
    const auto memory_lane = node.at("memory_lane").get<std::uint32_t>();
    const auto element_bits = node.at("element_bits").get<std::uint32_t>();
    const auto& memory = node.at("memory");

    parsed.operation = "memory_load";
    parsed.base_register = memory.at("base").get<std::string>();
    parsed.offset = memory.at("displacement").get<std::ptrdiff_t>();
    parsed.offset += static_cast<std::ptrdiff_t>(memory_lane * (element_bits / 8));
  } else if (parsed.kind == "binary") {
    parsed.operation = node.at("operation").get<std::string>();
  } else {
    throw std::runtime_error("unsupported raw expression node kind");
  }

  return parsed;
}

} // namespace

namespace sivra::compat {

loaded_expression_graph::loaded_expression_graph(
  std::shared_ptr<const ir::operation_catalogue> catalogue,
  ir::expression_graph graph,
  ir::node_id root,
  std::vector<raw_memory_operand> external_values
)
    : catalogue(std::move(catalogue)),
      graph(std::move(graph)),
      root(root),
      external_values(std::move(external_values)) {
}

core::result_t<loaded_expression_graph> parse_raw_expression_json(
  std::string_view json
) {
  try {
    const auto document = nlohmann::json::parse(json);
    if (document.at("format") != "simd-decompiler.raw-expression-dag.v1") {
      return compatibility_failure<loaded_expression_graph>(
        "unsupported raw expression JSON format"
      );
    }

    const auto root = document.at("root").get<std::uint32_t>();
    const auto& node_objects = document.at("nodes");

    std::vector<raw_node> raw_nodes;
    raw_nodes.reserve(node_objects.size());
    for (const auto& node : node_objects) {
      raw_nodes.push_back(parse_node(node));
    }

    std::ranges::sort(raw_nodes, {}, &raw_node::id);
    for (std::size_t index = 0; index < raw_nodes.size(); ++index) {
      if (raw_nodes[index].id != index) {
        return compatibility_failure<loaded_expression_graph>(
          "raw expression node ids must be unique and dense"
        );
      }
    }

    ir::operation_catalogue_builder catalogue_builder;
    auto builtins = propagate_ir_error(ir::register_builtin_operations(catalogue_builder));
    if (!builtins.has_value()) {
      return std::unexpected(std::move(builtins.error()));
    }
    auto catalogue = propagate_ir_error(std::move(catalogue_builder).freeze());
    if (!catalogue.has_value()) {
      return std::unexpected(std::move(catalogue.error()));
    }
    ir::expression_graph graph(*catalogue);
    ir::graph_builder builder(graph);
    std::vector<raw_memory_operand> external_values;

    std::vector<std::optional<ir::node_id>> mapped_nodes(raw_nodes.size());
    for (const auto& node : raw_nodes) {
      std::vector<ir::node_id> children;
      children.reserve(node.children.size());
      for (const auto child : node.children) {
        if (child >= mapped_nodes.size() || !mapped_nodes[child].has_value()) {
          return compatibility_failure<loaded_expression_graph>(
            "raw expression child id does not refer to an earlier node"
          );
        }
        children.push_back(*mapped_nodes[child]);
      }

      core::result_t<ir::node_id> mapped = [&] {
        if (node.operation == "memory_load") {
          external_values.push_back(
            raw_memory_operand{
              .base_register = node.base_register,
              .offset = node.offset,
            }
          );
          return builder.make_external_value(node.result_type);
        }
        if (node.operation == "multiply") {
          return builder.apply(builtins->multiply, children, node.result_type);
        }
        if (node.operation == "add") {
          return builder.apply(builtins->add, children, node.result_type);
        }
        if (node.operation == "subtract") {
          return builder.apply(builtins->subtract, children, node.result_type);
        }
        if (node.operation == "maximum") {
          return builder.apply(builtins->maximum, children, node.result_type);
        }
        return compatibility_failure<ir::node_id>("unsupported raw expression operation");
      }();
      if (!mapped.has_value()) {
        return std::unexpected(std::move(mapped.error()));
      }
      mapped_nodes[node.id] = *mapped;
    }

    if (root >= mapped_nodes.size() || !mapped_nodes[root].has_value()) {
      return compatibility_failure<loaded_expression_graph>(
        "raw expression root id does not refer to a node"
      );
    }

    return loaded_expression_graph(
      std::move(*catalogue), std::move(graph), *mapped_nodes[root], std::move(external_values)
    );
  } catch (const std::exception& error) {
    return compatibility_failure<loaded_expression_graph>(error.what());
  }
}

} // namespace sivra::compat
