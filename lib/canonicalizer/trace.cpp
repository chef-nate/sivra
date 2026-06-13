#include "trace.hpp"

#include <cstddef>
#include <utility>

namespace sivra::canonicalizer {

trace_collector::trace_collector(
  bool enabled
)
    : m_enabled(enabled) {
}

void trace_collector::record(
  const rewrite_rule& rule,
  std::optional<ir::structural_digest> old_root,
  std::optional<ir::structural_digest> new_root
) {
  if (!m_enabled) {
    return;
  }
  m_events.push_back(
    {
      .sequence = m_next_sequence++,
      .rule = rule.metadata.id,
      .phase = rule.metadata.phase,
      .old_root = old_root,
      .new_root = new_root,
      .reason = rule.metadata.description,
    }
  );
}

trace_collector::checkpoint trace_collector::mark() const {
  return {
    .event_count = m_events.size(),
    .next_sequence = m_next_sequence,
  };
}

void trace_collector::rollback(
  checkpoint saved
) {
  if (saved.event_count < m_events.size()) {
    m_events.erase(
      m_events.begin() + static_cast<std::ptrdiff_t>(saved.event_count), m_events.end()
    );
  }
  m_next_sequence = saved.next_sequence;
}

std::vector<rewrite_trace_event> trace_collector::take() {
  return std::move(m_events);
}

} // namespace sivra::canonicalizer
