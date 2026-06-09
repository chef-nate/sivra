#pragma once

#include <sivra/ir/expression_graph.hpp>

#include <vector>

namespace sivra::canonicalizer {

/**
 * @struct result
 * @brief Holds the result of canonicalizing one or more root expressions.
 *
 * roots preserve the order of the input root span passed to engine::canonicalize().
 * The graph references operations and types owned by the source graph's
 * ir_context, which must outlive this result.
 */
struct result {
  ir::expression_graph graph;
  std::vector<ir::node_id> roots;
};

/**
 * @struct single_result
 * @brief Holds the result of canonicalizing one root expression.
 *
 * The graph references operations and types owned by the source graph's
 * ir_context, which must outlive this result.
 */
struct single_result {
  ir::expression_graph graph;
  ir::node_id root;
};

} // namespace sivra::canonicalizer
