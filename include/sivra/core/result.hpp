#pragma once

#include "diagnostic.hpp"

#include <expected>
#include <optional>
#include <string>
#include <utility>

namespace sivra::core {

enum class analysis_status {
  complete,
  partial,
  unsupported,
  invalid_input,
  resource_exhausted,
  internal_failure,
};

template <typename T>
using result_t = std::expected<T, diagnostic_bundle_t>;

template <typename T>
struct stage_result {
  std::optional<T> artifact;
  analysis_status status = analysis_status::complete;
  diagnostic_bundle_t diagnostics;

  [[nodiscard]] bool has_artifact() const { return artifact.has_value(); }

  [[nodiscard]] bool is_consistent() const {
    const bool permits_artifact =
      status == analysis_status::complete || status == analysis_status::partial;
    return artifact.has_value() == permits_artifact;
  }
};

template <typename T>
result_t<T> fail(
  std::string code,
  std::string message
) {
  return std::unexpected(make_error(std::move(code), std::move(message)));
}

} // namespace sivra::core
