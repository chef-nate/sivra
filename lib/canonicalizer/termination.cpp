#include "termination.hpp"

#include <algorithm>
#include <utility>

namespace sivra::canonicalizer {

termination_tracker::termination_tracker(
  canonicalization_limits limits
)
    : m_limits(limits) {
}

std::optional<core::diagnostic> termination_tracker::consume_import() {
  if (m_statistics.imported_nodes >= m_limits.maximum_imported_nodes) {
    return exhausted("canonicalizer.import_budget", "canonicalizer imported-node budget exhausted");
  }
  ++m_statistics.imported_nodes;
  return std::nullopt;
}

std::optional<core::diagnostic> termination_tracker::consume_worklist_step() {
  if (m_statistics.worklist_steps >= m_limits.maximum_worklist_steps) {
    return exhausted(
      "canonicalizer.worklist_budget", "canonicalizer worklist-step budget exhausted"
    );
  }
  ++m_statistics.worklist_steps;
  return std::nullopt;
}

std::optional<core::diagnostic> termination_tracker::consume_rewrite(
  ir::node_id node
) {
  if (m_statistics.rewrites_applied >= m_limits.maximum_rewrites) {
    return exhausted("canonicalizer.rewrite_budget", "canonicalizer rewrite budget exhausted");
  }
  auto& node_rewrites = m_node_rewrites[node];
  if (node_rewrites >= m_limits.maximum_rewrites_per_node) {
    return exhausted(
      "canonicalizer.node_rewrite_budget", "canonicalizer per-node rewrite budget exhausted"
    );
  }
  ++node_rewrites;
  ++m_statistics.rewrites_applied;
  return std::nullopt;
}

std::optional<core::diagnostic> termination_tracker::begin_phase_iteration() {
  if (m_statistics.phase_iterations >= m_limits.maximum_phase_iterations) {
    return exhausted(
      "canonicalizer.phase_iteration_budget", "canonicalizer phase-iteration budget exhausted"
    );
  }
  ++m_statistics.phase_iterations;
  return std::nullopt;
}

std::optional<core::diagnostic> termination_tracker::observe_output_nodes(
  std::size_t nodes,
  std::size_t baseline
) {
  m_statistics.peak_output_nodes = std::max(m_statistics.peak_output_nodes, nodes);
  if (nodes > m_limits.maximum_output_nodes) {
    return exhausted("canonicalizer.output_budget", "canonicalizer output-node budget exhausted");
  }
  if (nodes > baseline && nodes - baseline > m_limits.maximum_node_growth) {
    return exhausted("canonicalizer.growth_budget", "canonicalizer node-growth budget exhausted");
  }
  return std::nullopt;
}

const canonicalization_statistics& termination_tracker::statistics() const {
  return m_statistics;
}

canonicalization_statistics& termination_tracker::statistics() {
  return m_statistics;
}

core::diagnostic termination_tracker::exhausted(
  std::string code,
  std::string message
) {
  m_statistics.exhausted_budget = true;
  return {
    .code = std::move(code),
    .severity = core::diagnostic_severity::error,
    .message = std::move(message),
  };
}

} // namespace sivra::canonicalizer
