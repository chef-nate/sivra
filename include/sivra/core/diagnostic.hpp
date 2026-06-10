#pragma once

#include <string>
#include <utility>
#include <vector>

namespace sivra::core {

enum class diagnostic_severity {
  note,
  warning,
  error,
  fatal,
};

struct diagnostic {
  std::string code;
  diagnostic_severity severity = diagnostic_severity::error;
  std::string message;
};

using diagnostic_bundle_t = std::vector<diagnostic>;

inline diagnostic_bundle_t make_error(
  std::string code,
  std::string message
) {
  return {
    diagnostic{
      .code = std::move(code),
      .severity = diagnostic_severity::error,
      .message = std::move(message),
    },
  };
}

} // namespace sivra::core
