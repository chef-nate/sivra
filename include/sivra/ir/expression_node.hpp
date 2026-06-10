#pragma once

#include "id.hpp"
#include "leaf.hpp"
#include "value_type.hpp"

#include <span>
#include <variant>
#include <vector>

namespace sivra::ir {

class expression_graph;

struct operation_application {
  operation_id operation;
  std::vector<node_id> operands;
};

using expression_payload_t = std::
  variant<constant_node, symbol_node, external_value_node, unknown_node, operation_application>;

enum class expression_node_kind {
  constant,
  symbol,
  external_value,
  unknown,
  operation,
};

class expression_node {
public:
  node_id id() const;
  const value_type& result_type() const;
  expression_node_kind kind() const;
  const expression_payload_t& payload() const;
  std::span<const node_id> operands() const;
  bool is_leaf() const;

  const constant_node* get_if_constant() const;
  const symbol_node* get_if_symbol() const;
  const external_value_node* get_if_external_value() const;
  const unknown_node* get_if_unknown() const;
  const operation_application* get_if_operation() const;

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
