#pragma once

#include "expression_node.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace sivra::ir {

/**
 * @class expression_graph
 * @brief Owns expression nodes and assigns stable node identifiers.
 *
 * expression_graph stores the nodes that make up an expression DAG. Nodes are
 * inserted into the graph and then referenced by node_id rather than by pointer.
 */
class expression_graph {
public:
  /**
   * @brief Adds a new expression node to the graph.
   *
   * @param operation Operation kind for the new node.
   * @param result_type Result type produced by the expression.
   * @param children Child expression nodes referenced by id.
   * @param leaf Optional leaf_type_t value for leaf expressions.
   * @return Stable id assigned to the inserted node.
   */
  node_id add_node(
    operation_id operation,
    type result_type,
    std::vector<node_id> children,
    std::optional<leaf_type_t> leaf = std::nullopt
  );

  /**
   * @brief Looks up a node by id.
   *
   * @param id node_id returned by add_node().
   * @return Const reference to the stored expression node.
   */
  const expression_node& at(
    node_id id
  ) const;

  /**
   * @brief Looks up a mutable node by id.
   *
   * @param id node_id returned by add_node().
   * @return Mutable reference to the stored expression node.
   */
  expression_node& at(
    node_id id
  );

  /**
   * @brief Returns the number of nodes stored in the graph.
   */
  std::size_t size() const;

private:
  std::vector<expression_node> m_nodes;
};

} // namespace sivra::ir
