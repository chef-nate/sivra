#include "rewrite.hpp"

namespace sivra::canonicalizer {

rewrite_context::rewrite_context(
  const ir::expression_graph& graph,
  const configuration& config,
  ir::structural_context& structural
)
    : m_graph(&graph),
      m_configuration(&config),
      m_structural(&structural) {
}

const ir::expression_graph& rewrite_context::graph() const {
  return *m_graph;
}

const ir::operation_catalogue& rewrite_context::catalogue() const {
  return m_graph->catalogue();
}

ir::structural_context& rewrite_context::structural() const {
  return *m_structural;
}

const ir::expression_node& rewrite_context::node(
  ir::node_id id
) const {
  return m_graph->at(id);
}

const ir::operation_def& rewrite_context::operation(
  ir::operation_id id
) const {
  return m_graph->catalogue().operation(id);
}

bool rewrite_context::is_trait_enabled(
  ir::operation_trait trait
) const {
  return m_configuration->is_trait_enabled(trait);
}

} // namespace sivra::canonicalizer
