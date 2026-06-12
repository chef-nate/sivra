#pragma once

#include "equivalence_contract.hpp"

#include <sivra/core/diagnostic.hpp>
#include <sivra/core/result.hpp>
#include <sivra/core/trace.hpp>
#include <sivra/ir/expression_graph.hpp>

#include <optional>
#include <vector>

namespace sivra::canonicalizer {

struct canonicalization_statistics {
  std::size_t imported_nodes = 0;
  std::size_t output_nodes = 0;
  std::size_t worklist_steps = 0;
  std::size_t rewrites_applied = 0;
};

class source_mapping {
public:
  source_mapping(
    core::owner_token source_owner,
    std::size_t source_size
  );

  void record(
    ir::node_id source,
    ir::node_id canonical
  );

  [[nodiscard]] std::optional<ir::node_id> canonical_for(
    ir::node_id source
  ) const;

  [[nodiscard]] std::size_t size() const;

private:
  core::owner_token m_source_owner;
  std::vector<std::optional<ir::node_id>> m_nodes;
};

struct canonicalization_result {
  ir::expression_graph graph;
  std::vector<ir::node_id> roots;
  equivalence_contract_id contract;
  core::analysis_status status = core::analysis_status::complete;
  core::diagnostic_bundle_t diagnostics;
  canonicalization_statistics statistics;
  source_mapping mapping;
  std::vector<core::trace_event> trace;
};

struct single_canonicalization_result {
  ir::expression_graph graph;
  std::optional<ir::node_id> root;
  equivalence_contract_id contract;
  core::analysis_status status = core::analysis_status::complete;
  core::diagnostic_bundle_t diagnostics;
  canonicalization_statistics statistics;
  source_mapping mapping;
  std::vector<core::trace_event> trace;
};

} // namespace sivra::canonicalizer
