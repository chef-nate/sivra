#pragma once

#include <sivra/canonicalizer/rewrite.hpp>
#include <sivra/canonicalizer/trace.hpp>

#include <vector>

namespace sivra::canonicalizer {

class trace_collector {
public:
  struct checkpoint {
    std::size_t event_count = 0;
    std::size_t next_sequence = 0;
  };

  explicit trace_collector(
    bool enabled
  );

  void record(
    const rewrite_rule& rule
  );

  [[nodiscard]] checkpoint mark() const;
  void rollback(
    checkpoint saved
  );
  [[nodiscard]] std::vector<rewrite_trace_event> take();

private:
  bool m_enabled;
  std::size_t m_next_sequence = 0;
  std::vector<rewrite_trace_event> m_events;
};

} // namespace sivra::canonicalizer
