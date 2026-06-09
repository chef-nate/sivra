#include <sivra/ir/expression_graph.hpp>

#include <limits>
#include <stdexcept>
#include <utility>
#include <variant>

namespace sivra::ir {

expression_graph::expression_graph(
  const ir_context& context
)
    : m_context(&context) {
}

const ir_context& expression_graph::context() const {
  return *m_context;
}

node_id expression_graph::add_node(
  operation_id operation,
  const type& result_type,
  std::vector<node_id> children,
  std::optional<leaf_type_t> leaf_value
) {
  m_context->operations().at(operation);

  if (&result_type.context() != &m_context->types()) {
    throw std::invalid_argument("result type belongs to another ir_context");
  }

  if (leaf_value.has_value()) {
    const auto* constant = std::get_if<constant_value>(&*leaf_value);
    if (constant != nullptr && &constant->result_type() != &result_type) {
      throw std::invalid_argument("constant value type does not match node result type");
    }
  }

  // std::vector::size() may exceed uint32_t on 64-bit targets, so check before
  // narrowing the size into a node_id.
  if (m_nodes.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("expression_graph node_id limit exceeded");
  }

  const node_id id(static_cast<std::uint32_t>(m_nodes.size()));
  m_nodes.emplace_back(id, operation, result_type, std::move(children), std::move(leaf_value));
  return id;
}

node_id expression_graph::add_constant(
  operation_id operation,
  constant_value value
) {
  const auto& result_type = value.result_type();
  return add_node(operation, result_type, {}, std::move(value));
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
