#pragma once

#include "expression_node.hpp"
#include "operation_catalogue.hpp"

#include <memory>
#include <span>
#include <vector>

namespace sivra::ir {

class graph_builder;

class expression_graph {
public:
  explicit expression_graph(
    std::shared_ptr<const operation_catalogue> catalogue
  );

  const expression_node& at(
    node_id id
  ) const;

  const expression_node& node(
    node_id id
  ) const;

  std::span<const expression_node> nodes() const;
  std::size_t size() const;
  bool contains(
    node_id id
  ) const;

  const operation_catalogue& catalogue() const;
  std::shared_ptr<const operation_catalogue> shared_catalogue() const;
  core::owner_token owner() const;

private:
  friend class graph_builder;

  node_id append_validated(
    value_type result_type,
    expression_payload_t payload
  );

  external_value_id allocate_external_value_id();

  core::owner_token m_owner;
  std::shared_ptr<const operation_catalogue> m_catalogue;
  std::vector<expression_node> m_nodes;
  std::uint32_t m_next_external_value = 0;
};

} // namespace sivra::ir
