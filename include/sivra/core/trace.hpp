#pragma once

#include <string>

namespace sivra::core {

struct trace_event {
  std::string domain;
  std::string kind;
  std::string detail;
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
  ) override {}
};

} // namespace sivra::core
