#include <sivra/ir/expression_node.hpp>

#include <utility>

namespace sivra::ir {

expression_node::expression_node(
  node_id id,
  operation_id operation,
  type result_type,
  std::vector<node_id> children,
  std::optional<leaf_type> leaf
)
    : m_node_id(id),
      m_operation_id(operation),
      m_type(result_type),
      m_children(std::move(children)),
      m_leaf(std::move(leaf)) {
}

node_id expression_node::id() const {
  return m_node_id;
}

operation_id expression_node::operation() const {
  return m_operation_id;
}

const type& expression_node::result_type() const {
  return m_type;
}

std::span<const node_id> expression_node::children() const {
  return m_children;
}

const std::optional<leaf_type>& expression_node::leaf() const {
  return m_leaf;
}

bool expression_node::is_leaf() const {
  return m_children.empty();
}

} // namespace sivra::ir
