#pragma once

#include "id.hpp"
#include "leaf.hpp"
#include "operation_attribute.hpp"
#include "value_type.hpp"

#include <span>
#include <variant>
#include <vector>

namespace sivra::ir {

class expression_graph;

struct operation_application {
  operation_id operation;
  std::vector<node_id> operands;
  operation_attributes attributes;
};

struct merge_node {
  std::vector<node_id> incoming;
};

using expression_payload_t = std::variant<
  constant_node,
  symbol_node,
  external_value_node,
  unknown_node,
  operation_application,
  merge_node
>;

enum class expression_node_kind {
  constant,
  symbol,
  external_value,
  unknown,
  operation,
  merge,
};

class expression_node {
public:
  [[nodiscard]] node_id id() const;
  [[nodiscard]] const value_type& result_type() const;
  [[nodiscard]] expression_node_kind kind() const;
  [[nodiscard]] const expression_payload_t& payload() const;
  [[nodiscard]] std::span<const node_id> operands() const;
  [[nodiscard]] bool is_leaf() const;

  [[nodiscard]] const constant_node* get_if_constant() const;
  [[nodiscard]] const symbol_node* get_if_symbol() const;
  [[nodiscard]] const external_value_node* get_if_external_value() const;
  [[nodiscard]] const unknown_node* get_if_unknown() const;
  [[nodiscard]] const operation_application* get_if_operation() const;
  [[nodiscard]] const merge_node* get_if_merge() const;

private:
  friend class expression_graph;

  expression_node(
    node_id id,
    value_type result_type,
    expression_payload_t payload
  );

  node_id m_id;
  value_type m_result_type;
  expression_payload_t m_payload;
};

} // namespace sivra::ir
