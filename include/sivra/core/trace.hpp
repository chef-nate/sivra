#pragma once

#include "source_location.hpp"

#include <optional>
#include <string>

namespace sivra::core {

enum class trace_domain {
  core,
  ir,
  canonicalizer,
  compatibility,
};

struct trace_event {
  trace_domain domain = trace_domain::core;
  std::string kind;
  std::string detail;
  std::optional<source_span> source;
};

class trace_sink {
public:
  virtual ~trace_sink() = default;
  virtual void emit(
    const trace_event& event
  ) = 0;
};

class null_trace_sink final : public trace_sink {
public:
  void emit(
    const trace_event&
  ) override;
};

} // namespace sivra::core
