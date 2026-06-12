#include <sivra/ir/expression_node.hpp>

#include <type_traits>
#include <utility>

namespace sivra::ir {

expression_node::expression_node(
  node_id id,
  value_type result_type,
  expression_payload_t payload
)
    : m_id(id),
      m_result_type(std::move(result_type)),
      m_payload(std::move(payload)) {
}

node_id expression_node::id() const {
  return m_id;
}

const value_type& expression_node::result_type() const {
  return m_result_type;
}

expression_node_kind expression_node::kind() const {
  return std::visit(
    []<typename T>(const T&) {
      if constexpr (std::is_same_v<T, constant_node>) {
        return expression_node_kind::constant;
      } else if constexpr (std::is_same_v<T, symbol_node>) {
        return expression_node_kind::symbol;
      } else if constexpr (std::is_same_v<T, external_value_node>) {
        return expression_node_kind::external_value;
      } else if constexpr (std::is_same_v<T, unknown_node>) {
        return expression_node_kind::unknown;
      } else if constexpr (std::is_same_v<T, operation_application>) {
        return expression_node_kind::operation;
      } else {
        return expression_node_kind::merge;
      }
    },
    m_payload
  );
}

const expression_payload_t& expression_node::payload() const {
  return m_payload;
}

std::span<const node_id> expression_node::operands() const {
  if (const auto* operation = get_if_operation()) {
    return operation->operands;
  }
  if (const auto* merge = get_if_merge()) {
    return merge->incoming;
  }
  return {};
}

bool expression_node::is_leaf() const {
  return operands().empty();
}

const constant_node* expression_node::get_if_constant() const {
  return std::get_if<constant_node>(&m_payload);
}

const symbol_node* expression_node::get_if_symbol() const {
  return std::get_if<symbol_node>(&m_payload);
}

const external_value_node* expression_node::get_if_external_value() const {
  return std::get_if<external_value_node>(&m_payload);
}

const unknown_node* expression_node::get_if_unknown() const {
  return std::get_if<unknown_node>(&m_payload);
}

const operation_application* expression_node::get_if_operation() const {
  return std::get_if<operation_application>(&m_payload);
}

const merge_node* expression_node::get_if_merge() const {
  return std::get_if<merge_node>(&m_payload);
}

} // namespace sivra::ir
