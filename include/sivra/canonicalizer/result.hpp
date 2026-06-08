#pragma once

#include <sivra/ir/expression_graph.hpp>

#include <vector>

namespace sivra::canonicalizer {

/**
 * @struct result
 * @brief Holds the result of canonicalizing one or more root expressions.
 *
 * roots preserve the order of the input root span passed to engine::canonicalize().
 */
struct result {
  ir::expression_graph graph;
  std::vector<ir::node_id> roots;
};

/**
 * @struct single_result
 * @brief Holds the result of canonicalizing one root expression.
 */
struct single_result {
  ir::expression_graph graph;
  ir::node_id root;
};

} // namespace sivra::canonicalizer
