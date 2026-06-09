#pragma once

#include "expression_node.hpp"
#include "ir_context.hpp"

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
 * The graph does not own its ir_context; the context must outlive the graph.
 */
class expression_graph {
public:
  explicit expression_graph(
    const ir_context& context
  );

  /**
   * @brief Returns the context that owns this graph's operations and types.
   */
  const ir_context& context() const;

  /**
   * @brief Adds a new expression node to the graph.
   *
   * @param operation Operation kind for the new node.
   * @param result_type Result type produced by the expression.
   * @param children Child expression nodes referenced by id.
   * @param leaf_value Optional leaf_type_t value associated with the new node.
   * @return Stable id assigned to the inserted node.
   * @throws std::out_of_range if operation is not registered in the graph context.
   * @throws std::invalid_argument if result_type belongs to another context, a
   * constant value does not match result_type, or a child does not identify an
   * existing node in this graph.
   * @throws std::length_error if the node_id capacity is exhausted.
   */
  node_id add_node(
    operation_id operation,
    const type& result_type,
    std::vector<node_id> children,
    std::optional<leaf_type_t> leaf_value = std::nullopt
  );

  /**
   * @brief Adds a constant expression using the constant's result type.
   */
  node_id add_constant(
    operation_id operation,
    constant_value value
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
   * @brief Returns the number of nodes stored in the graph.
   */
  std::size_t size() const;

private:
  const ir_context* m_context;
  std::vector<expression_node> m_nodes;
};

} // namespace sivra::ir
