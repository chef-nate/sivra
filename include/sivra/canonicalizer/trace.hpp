#pragma once

#include "phase.hpp"

#include <sivra/ir/structural.hpp>

#include <cstddef>
#include <optional>
#include <string>

namespace sivra::canonicalizer {

struct rewrite_trace_event {
  std::size_t sequence = 0;
  rule_id rule;
  pass_phase phase = pass_phase::local_simplification;
  std::optional<ir::structural_digest> old_root;
  std::optional<ir::structural_digest> new_root;
  std::string reason;
};

} // namespace sivra::canonicalizer
