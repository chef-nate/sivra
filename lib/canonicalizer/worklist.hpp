#pragma once

#include "termination.hpp"
#include "trace.hpp"

#include <sivra/canonicalizer/phase.hpp>
#include <sivra/canonicalizer/rewrite.hpp>
#include <sivra/core/result.hpp>

#include <vector>

namespace sivra::canonicalizer {

struct worklist_result {
  ir::expression_graph graph;
  std::vector<ir::node_id> roots;
  source_mapping mapping;
  bool changed = false;
  core::analysis_status status = core::analysis_status::complete;
  core::diagnostic_bundle_t diagnostics;
};

class worklist_engine {
public:
  worklist_engine(
    const configuration& configuration,
    const rule_catalogue& rules,
    const evaluator_catalogue& evaluators,
    termination_tracker& termination,
    trace_collector& trace
  );

  [[nodiscard]] worklist_result run(
    const ir::expression_graph& source,
    std::span<const ir::node_id> roots,
    const pass_plan* plan,
    bool count_imports
  );

private:
  const configuration* m_configuration;
  const rule_catalogue* m_rules;
  const evaluator_catalogue* m_evaluators;
  termination_tracker* m_termination;
  trace_collector* m_trace;
};

} // namespace sivra::canonicalizer
