#include "raw_expression_json.hpp"

#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/scalar_type.hpp>

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

struct raw_node {
  std::uint32_t id;
  std::string kind;
  std::string operation;
  sivra::ir::scalar_type scalar;
  std::string base_register;
  std::ptrdiff_t offset = 0;
  std::vector<std::uint32_t> children;
};

sivra::ir::scalar_type parse_scalar_type(
  const nlohmann::json& node
) {
  const auto& type = node.at("type");
  const auto scalar = type.at("scalar").get<std::string>();
  const auto element_bits = type.at("element_bits").get<std::uint32_t>();
  const auto element_count = type.at("lane_count").get<std::uint32_t>();

  if (scalar == "floating_point" && element_bits == 32 && element_count == 1) {
    return sivra::ir::scalar_type::f32;
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
    .scalar = parse_scalar_type(node),
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

namespace sivra::tool {

loaded_expression_graph::loaded_expression_graph()
    : context(std::make_unique<ir::ir_context>()),
      graph(*context),
      root(0) {
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

  loaded_expression_graph loaded;

  auto& context = *loaded.context;
  const auto builtins = ir::register_builtin_operations(context.operations());
  const auto subtract = context.operations().register_operation("subtract");
  const auto maximum = context.operations().register_operation("maximum");

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

    auto operation = builtins.memory_load;
    std::optional<ir::leaf_type_t> leaf_value;
    if (node.operation == "memory_load") {
      leaf_value = ir::memory_ref{
        .scalar = node.scalar,
        .base_register = node.base_register,
        .offset = node.offset,
      };
    } else if (node.operation == "multiply") {
      operation = builtins.multiply;
    } else if (node.operation == "add") {
      operation = builtins.add;
    } else if (node.operation == "subtract") {
      operation = subtract;
    } else if (node.operation == "maximum") {
      operation = maximum;
    } else {
      throw std::runtime_error("unsupported raw expression operation");
    }

    const auto& result_type = context.types().scalar(node.scalar);
    mapped_nodes[node.id] =
      loaded.graph.add_node(operation, result_type, std::move(children), std::move(leaf_value));
  }

  if (root >= mapped_nodes.size() || !mapped_nodes[root].has_value()) {
    throw std::runtime_error("raw expression root id does not refer to a node");
  }

  loaded.root = *mapped_nodes[root];
  return loaded;
}

} // namespace sivra::tool
