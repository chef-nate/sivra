#include <sivra/ir/expression_graph.hpp>

#include <limits>
#include <stdexcept>
#include <utility>

namespace sivra::ir {

expression_graph::expression_graph(
  std::shared_ptr<const operation_catalogue> catalogue
)
    : m_owner(core::owner_token_source::next()),
      m_catalogue(std::move(catalogue)) {
  if (m_catalogue == nullptr) {
    throw std::invalid_argument("expression_graph requires an operation catalogue");
  }
}

const expression_node& expression_graph::at(
  node_id id
) const {
  if (id.owner() != m_owner) {
    throw std::invalid_argument("node_id belongs to another expression_graph");
  }
  return m_nodes.at(id.index());
}

const expression_node& expression_graph::node(
  node_id id
) const {
  return at(id);
}

std::span<const expression_node> expression_graph::nodes() const {
  return m_nodes;
}

std::size_t expression_graph::size() const {
  return m_nodes.size();
}

bool expression_graph::contains(
  node_id id
) const {
  return id.owner() == m_owner && id.index() < m_nodes.size();
}

const operation_catalogue& expression_graph::catalogue() const {
  return *m_catalogue;
}

std::shared_ptr<const operation_catalogue> expression_graph::shared_catalogue() const {
  return m_catalogue;
}

core::owner_token expression_graph::owner() const {
  return m_owner;
}

node_id expression_graph::append_validated(
  value_type result_type,
  expression_payload_t payload
) {
  if (m_nodes.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("expression_graph node_id limit exceeded");
  }
  const auto id = node_id::unsafe_from_index(static_cast<std::uint32_t>(m_nodes.size()), m_owner);
  m_nodes.push_back(expression_node(id, std::move(result_type), std::move(payload)));
  return id;
}

external_value_id expression_graph::allocate_external_value_id() {
  if (m_next_external_value == std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("expression_graph external_value_id limit exceeded");
  }
  return external_value_id::unsafe_from_index(m_next_external_value++, m_catalogue->owner());
}

} // namespace sivra::ir
