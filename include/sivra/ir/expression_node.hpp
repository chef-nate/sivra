#pragma once

#include "id.hpp"
#include "leaf.hpp"
#include "type.hpp"

#include <optional>
#include <span>
#include <vector>

namespace sivra::ir {

/**
 * @class expression_node
 * @brief Represents one node in an expression DAG.
 *
 * expression_node stores the operation, result type, child node identifiers,
 * and optional leaf_type value for a single expression.
 */
class expression_node {
public:
  /**
   * @brief Creates an expression node.
   *
   * @param id node_id assigned by the owning expression_graph.
   * @param operation operation_id describing the node's operation.
   * @param result_type Type produced by this expression.
   * @param children Child expressions referenced by node_id.
   * @param leaf Optional leaf_type value for leaf expressions.
   */
  expression_node(
    node_id id,
    operation_id operation,
    type result_type,
    std::vector<node_id> children,
    std::optional<leaf_type> leaf = std::nullopt
  );

  /**
   * @brief Returns this node's graph-assigned node_id.
   */
  node_id id() const;

  /**
   * @brief Returns the operation_id for this node.
   */
  operation_id operation() const;

  /**
   * @brief Returns the result type produced by this expression.
   */
  const type& result_type() const;

  /**
   * @brief Returns child node identifiers in operand order.
   */
  std::span<const node_id> children() const;

  /**
   * @brief Returns the optional leaf_type value stored by this node.
   */
  const std::optional<leaf_type>& leaf() const;

  /**
   * @brief Returns true when this node has no child expressions.
   */
  bool is_leaf() const;

private:
  node_id m_node_id;
  operation_id m_operation_id;
  type m_type;

  std::vector<node_id> m_children;
  std::optional<leaf_type> m_leaf;
};

} // namespace sivra::ir
