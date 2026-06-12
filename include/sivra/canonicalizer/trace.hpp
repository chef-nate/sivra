#pragma once

#include "phase.hpp"

#include <cstddef>
#include <string>

namespace sivra::canonicalizer {

struct rewrite_trace_event {
  std::size_t sequence = 0;
  rule_id rule;
  pass_phase phase = pass_phase::local_simplification;
  std::string reason;
};

} // namespace sivra::canonicalizer
