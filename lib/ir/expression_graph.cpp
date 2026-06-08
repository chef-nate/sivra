#include <sivra/ir/expression_graph.hpp>

#include <utility>

namespace sivra::ir {

node_id expression_graph::add_node(
  operation_id operation,
  type result_type,
  std::vector<node_id> children,
  std::optional<leaf_type_t> leaf
) {
  const node_id id(static_cast<std::uint32_t>(m_nodes.size()));
  m_nodes.emplace_back(id, operation, result_type, std::move(children), std::move(leaf));
  return id;
}

const expression_node& expression_graph::at(
  node_id id
) const {
  return m_nodes.at(id.value());
}

expression_node& expression_graph::at(
  node_id id
) {
  return m_nodes.at(id.value());
}

std::size_t expression_graph::size() const {
  return m_nodes.size();
}

} // namespace sivra::ir
