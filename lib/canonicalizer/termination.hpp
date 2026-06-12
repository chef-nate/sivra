#pragma once

#include <sivra/canonicalizer/configuration.hpp>
#include <sivra/canonicalizer/result.hpp>
#include <sivra/core/diagnostic.hpp>
#include <sivra/ir/id.hpp>

#include <optional>
#include <unordered_map>

namespace sivra::canonicalizer {

class termination_tracker {
public:
  explicit termination_tracker(
    canonicalization_limits limits
  );

  [[nodiscard]] std::optional<core::diagnostic> consume_import();
  [[nodiscard]] std::optional<core::diagnostic> consume_worklist_step();
  [[nodiscard]] std::optional<core::diagnostic> consume_rewrite(
    ir::node_id node
  );
  [[nodiscard]] std::optional<core::diagnostic> begin_phase_iteration();
  [[nodiscard]] std::optional<core::diagnostic> observe_output_nodes(
    std::size_t nodes,
    std::size_t baseline
  );

  [[nodiscard]] const canonicalization_statistics& statistics() const;
  [[nodiscard]] canonicalization_statistics& statistics();

private:
  [[nodiscard]] core::diagnostic exhausted(
    std::string code,
    std::string message
  );

  canonicalization_limits m_limits;
  canonicalization_statistics m_statistics;
  std::unordered_map<ir::node_id, std::size_t> m_node_rewrites;
};

} // namespace sivra::canonicalizer
