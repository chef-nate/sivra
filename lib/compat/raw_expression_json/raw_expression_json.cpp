#include "raw_expression_json.hpp"

#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/graph_builder.hpp>
#include <sivra/ir/value_type.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

template <typename T>
T value_or_throw(
  sivra::core::result_t<T> result
) {
  if (!result.has_value()) {
    const auto message =
      result.error().empty() ? "IR graph construction failed" : result.error().front().message;
    throw std::runtime_error(message);
  }
  return std::move(*result);
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
  std::vector<legacy_memory_ref> external_values
)
    : catalogue(std::move(catalogue)),
      graph(std::move(graph)),
      root(root),
      external_values(std::move(external_values)) {
}

loaded_expression_graph parse_raw_expression_json(
  std::string_view json
) {
  const auto document = nlohmann::json::parse(json);
  if (document.at("format") != "simd-decompiler.raw-expression-dag.v1") {
    throw std::runtime_error("unsupported raw expression JSON format");
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
      throw std::runtime_error("raw expression node ids must be unique and dense");
    }
  }

  ir::operation_catalogue_builder catalogue_builder;
  const auto builtins = value_or_throw(ir::register_builtin_operations(catalogue_builder));
  auto catalogue = value_or_throw(std::move(catalogue_builder).freeze());
  ir::expression_graph graph(catalogue);
  ir::graph_builder builder(graph);
  std::vector<legacy_memory_ref> external_values;

  std::vector<std::optional<ir::node_id>> mapped_nodes(raw_nodes.size());
  for (const auto& node : raw_nodes) {
    std::vector<ir::node_id> children;
    children.reserve(node.children.size());
    for (const auto child : node.children) {
      if (child >= mapped_nodes.size() || !mapped_nodes[child].has_value()) {
        throw std::runtime_error("raw expression child id does not refer to an earlier node");
      }
      children.push_back(*mapped_nodes[child]);
    }

    auto mapped = ir::node_id::unsafe_from_index(0, graph.owner());
    if (node.operation == "memory_load") {
      mapped = value_or_throw(builder.make_external_value(node.result_type));
      external_values.push_back(
        legacy_memory_ref{
          .base_register = node.base_register,
          .offset = node.offset,
        }
      );
    } else if (node.operation == "multiply") {
      mapped = value_or_throw(builder.apply(builtins.multiply, children, node.result_type));
    } else if (node.operation == "add") {
      mapped = value_or_throw(builder.apply(builtins.add, children, node.result_type));
    } else if (node.operation == "subtract") {
      mapped = value_or_throw(builder.apply(builtins.subtract, children, node.result_type));
    } else if (node.operation == "maximum") {
      mapped = value_or_throw(builder.apply(builtins.maximum, children, node.result_type));
    } else {
      throw std::runtime_error("unsupported raw expression operation");
    }
    mapped_nodes[node.id] = mapped;
  }

  if (root >= mapped_nodes.size() || !mapped_nodes[root].has_value()) {
    throw std::runtime_error("raw expression root id does not refer to a node");
  }

  return loaded_expression_graph(
    std::move(catalogue), std::move(graph), *mapped_nodes[root], std::move(external_values)
  );
}

} // namespace sivra::compat
