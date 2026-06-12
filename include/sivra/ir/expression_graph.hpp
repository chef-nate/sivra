#pragma once

#include "expression_node.hpp"
#include "operation_catalogue.hpp"

#include <sivra/core/result.hpp>

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sivra::ir {

class graph_builder;

class expression_graph {
public:
  explicit expression_graph(
    std::shared_ptr<const operation_catalogue> catalogue
  );

  [[nodiscard]] const expression_node& at(
    node_id id
  ) const;

  [[nodiscard]] const expression_node& node(
    node_id id
  ) const;

  [[nodiscard]] std::span<const expression_node> nodes() const;
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] bool contains(
    node_id id
  ) const;

  [[nodiscard]] const operation_catalogue& catalogue() const;
  [[nodiscard]] std::shared_ptr<const operation_catalogue> shared_catalogue() const;
  [[nodiscard]] core::owner_token owner() const;
  [[nodiscard]] core::result_t<void> validate() const;
  [[nodiscard]] std::string_view symbol_name(
    symbol_id symbol
  ) const;

private:
  friend class graph_builder;

  node_id append_validated(
    value_type result_type,
    expression_payload_t payload
  );

  external_value_id allocate_external_value_id();
  symbol_id allocate_symbol_id(
    std::string name
  );

  core::owner_token m_owner;
  std::shared_ptr<const operation_catalogue> m_catalogue;
  std::vector<expression_node> m_nodes;
  std::vector<std::string> m_symbol_names;
  std::uint32_t m_next_external_value = 0;
};

} // namespace sivra::ir
