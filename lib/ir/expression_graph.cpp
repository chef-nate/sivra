#include <sivra/ir/expression_graph.hpp>

#include <limits>
#include <stdexcept>
#include <utility>

namespace sivra::ir {

node_id expression_graph::add_node(
  operation_id operation,
  const type& result_type,
  std::vector<node_id> children,
  std::optional<leaf_type_t> leaf_value
) {
  // std::vector::size() may exceed uint32_t on 64-bit targets, so check before
  // narrowing the size into a node_id.
  if (m_nodes.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("expression_graph node_id limit exceeded");
  }

  const node_id id(static_cast<std::uint32_t>(m_nodes.size()));
  m_nodes.emplace_back(id, operation, result_type, std::move(children), std::move(leaf_value));
  return id;
}

const expression_node& expression_graph::at(
  node_id id
) const {
  return m_nodes.at(id.value());
}

std::size_t expression_graph::size() const {
  return m_nodes.size();
}

} // namespace sivra::ir
