#include <sivra/ir/structural.hpp>

#include <sivra/core/hash.hpp>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <variant>

namespace {

void append_u8(
  std::vector<std::byte>& output,
  std::uint8_t value
) {
  output.push_back(static_cast<std::byte>(value));
}

void append_u32(
  std::vector<std::byte>& output,
  std::uint32_t value
) {
  for (unsigned shift = 0; shift < 32; shift += 8) {
    append_u8(output, static_cast<std::uint8_t>(value >> shift));
  }
}

void append_u64(
  std::vector<std::byte>& output,
  std::uint64_t value
) {
  for (unsigned shift = 0; shift < 64; shift += 8) {
    append_u8(output, static_cast<std::uint8_t>(value >> shift));
  }
}

void append_bytes(
  std::vector<std::byte>& output,
  std::span<const std::byte> bytes
) {
  append_u64(output, bytes.size());
  output.insert(output.end(), bytes.begin(), bytes.end());
}

void append_string(
  std::vector<std::byte>& output,
  std::string_view value
) {
  append_u64(output, value.size());
  for (const auto character : value) {
    append_u8(output, static_cast<std::uint8_t>(character));
  }
}

void append_type(
  std::vector<std::byte>& output,
  const sivra::ir::value_type& type
) {
  append_u8(output, static_cast<std::uint8_t>(type.kind()));
  append_u8(output, static_cast<std::uint8_t>(type.category()));
  append_u32(output, type.element_bit_width());
  append_u32(output, type.lane_count());
}

void append_constant(
  std::vector<std::byte>& output,
  const sivra::ir::constant_value& value
) {
  append_u8(output, value.is_splat() ? 1 : 0);
  append_u64(output, value.element_count());
  for (std::size_t index = 0; index < value.element_count(); ++index) {
    std::visit(
      [&output](const auto& element) {
        using element_t = std::remove_cvref_t<decltype(element)>;
        if constexpr (std::is_same_v<element_t, sivra::ir::f32_constant>) {
          append_u8(output, 0);
        } else {
          append_u8(output, 1);
        }
        append_u32(output, element.bits);
      },
      value.element(index)
    );
  }
}

void append_attribute_value(
  std::vector<std::byte>& output,
  const sivra::ir::operation_attribute_value& value
) {
  append_u8(output, static_cast<std::uint8_t>(value.index()));
  std::visit(
    [&output](const auto& entry) {
      using entry_t = std::remove_cvref_t<decltype(entry)>;
      if constexpr (std::is_same_v<entry_t, std::int64_t>) {
        append_u64(output, static_cast<std::uint64_t>(entry));
      } else if constexpr (std::is_same_v<entry_t, bool>) {
        append_u8(output, entry ? 1 : 0);
      } else if constexpr (std::is_same_v<entry_t, sivra::ir::operation_enum_value>) {
        append_string(output, entry.key());
      } else if constexpr (std::is_same_v<entry_t, std::vector<std::uint32_t>>) {
        append_u64(output, entry.size());
        for (const auto index : entry) {
          append_u32(output, index);
        }
      } else {
        append_type(output, entry);
      }
    },
    value
  );
}

void append_attributes(
  std::vector<std::byte>& output,
  const sivra::ir::operation_attributes& attributes
) {
  append_u64(output, attributes.entries().size());
  for (const auto& attribute : attributes.entries()) {
    append_string(output, attribute.key);
    append_attribute_value(output, attribute.value);
  }
}

std::strong_ordering compare_bytes(
  std::span<const std::byte> lhs,
  std::span<const std::byte> rhs
) {
  const auto comparison = std::lexicographical_compare_three_way(
    lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](std::byte lhs_byte, std::byte rhs_byte) {
      return std::to_integer<unsigned>(lhs_byte) <=> std::to_integer<unsigned>(rhs_byte);
    }
  );
  return comparison;
}

} // namespace

namespace sivra::ir {

structural_digest structural_context::hash(
  const expression_graph& graph,
  node_id root
) {
  const auto& bytes = encoding(graph, root);
  std::size_t forward = 0;
  std::size_t reverse = 0;
  for (const auto byte : bytes) {
    core::hash_combine(forward, std::to_integer<std::uint8_t>(byte));
  }
  core::hash_combine(reverse, bytes.size());
  for (auto iterator = bytes.rbegin(); iterator != bytes.rend(); ++iterator) {
    core::hash_combine(reverse, std::to_integer<std::uint8_t>(*iterator));
  }
  return structural_digest{.words = {forward, reverse}};
}

bool structural_context::equal(
  const expression_graph& lhs_graph,
  node_id lhs,
  const expression_graph& rhs_graph,
  node_id rhs
) {
  return encoding(lhs_graph, lhs) == encoding(rhs_graph, rhs);
}

std::strong_ordering structural_context::compare(
  const expression_graph& lhs_graph,
  node_id lhs,
  const expression_graph& rhs_graph,
  node_id rhs
) {
  return compare_bytes(encoding(lhs_graph, lhs), encoding(rhs_graph, rhs));
}

const std::vector<std::byte>& structural_context::encoding(
  const expression_graph& graph,
  node_id root
) {
  static_cast<void>(graph.at(root));
  auto& graph_encodings = m_encodings[&graph];
  graph_encodings.reserve(graph.size());

  while (graph_encodings.size() < graph.size()) {
    const auto index = static_cast<std::uint32_t>(graph_encodings.size());
    const auto id = node_id::unsafe_from_index(index, graph.owner());
    const auto& node = graph.at(id);
    std::vector<std::byte> encoded;
    append_u8(encoded, static_cast<std::uint8_t>(node.kind()));
    append_type(encoded, node.result_type());

    if (const auto* constant = node.get_if_constant()) {
      append_constant(encoded, constant->value);
    } else if (const auto* symbol = node.get_if_symbol()) {
      append_string(encoded, graph.symbol_name(symbol->symbol));
    } else if (const auto* external = node.get_if_external_value()) {
      append_u32(encoded, external->value.index());
    } else if (const auto* unknown = node.get_if_unknown()) {
      append_string(encoded, unknown->reason);
    } else if (const auto* application = node.get_if_operation()) {
      append_string(encoded, graph.catalogue().operation(application->operation).key());
      append_u32(
        encoded, graph.catalogue().operation(application->operation).stable_key().version()
      );
      append_attributes(encoded, application->attributes);
      append_u64(encoded, application->operands.size());
      for (const auto operand : application->operands) {
        if (operand.owner() != graph.owner() || operand.index() >= index) {
          throw std::invalid_argument("structural encoding requires an ordered expression DAG");
        }
        append_bytes(encoded, graph_encodings[operand.index()]);
      }
    } else if (const auto* merge = node.get_if_merge()) {
      append_u64(encoded, merge->incoming.size());
      for (const auto incoming : merge->incoming) {
        if (incoming.owner() != graph.owner() || incoming.index() >= index) {
          throw std::invalid_argument("structural encoding requires an ordered expression DAG");
        }
        append_bytes(encoded, graph_encodings[incoming.index()]);
      }
    } else {
      throw std::logic_error("expression node has no structural payload");
    }

    graph_encodings.push_back(std::move(encoded));
  }

  return graph_encodings[root.index()];
}

} // namespace sivra::ir
