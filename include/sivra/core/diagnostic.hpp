#pragma once

#include "source_location.hpp"

#include <compare>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sivra::core {

class diagnostic_code {
public:
  diagnostic_code() = default;
  diagnostic_code(
    std::string value
  );
  diagnostic_code(
    const char* value
  );

  [[nodiscard]] std::string_view domain() const;
  [[nodiscard]] std::string_view value() const;
  [[nodiscard]] bool empty() const;

  auto operator<=>(
    const diagnostic_code&
  ) const = default;

  bool operator==(
    const diagnostic_code&
  ) const = default;

  [[nodiscard]] bool operator==(
    std::string_view value
  ) const;

private:
  std::string m_value;
};

[[nodiscard]] bool operator==(
  std::string_view lhs,
  const diagnostic_code& rhs
);

enum class diagnostic_severity {
  note,
  warning,
  error,
  fatal,
};

struct diagnostic_note {
  std::string message;
  std::optional<source_span> source;

  bool operator==(
    const diagnostic_note&
  ) const = default;
};

struct diagnostic {
  diagnostic_code code;
  diagnostic_severity severity = diagnostic_severity::error;
  std::string stage;
  std::string message;
  std::optional<source_span> source;
  std::vector<diagnostic_note> notes;

  diagnostic& with_note(
    diagnostic_note note
  );
};

using diagnostic_bundle_t = std::vector<diagnostic>;

[[nodiscard]] diagnostic_bundle_t make_error(
  std::string code,
  std::string message
);

} // namespace sivra::core
