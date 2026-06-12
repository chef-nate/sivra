#include <sivra/core/diagnostic.hpp>

#include <utility>

namespace sivra::core {

diagnostic_code::diagnostic_code(
  std::string value
)
    : m_value(std::move(value)) {
}

diagnostic_code::diagnostic_code(
  const char* value
)
    : m_value(value) {
}

std::string_view diagnostic_code::domain() const {
  const auto separator = m_value.find('.');
  return separator == std::string::npos ? std::string_view(m_value)
                                        : std::string_view(m_value).substr(0, separator);
}

std::string_view diagnostic_code::value() const {
  return m_value;
}

bool diagnostic_code::empty() const {
  return m_value.empty();
}

bool diagnostic_code::operator==(
  std::string_view value
) const {
  return std::string_view(m_value) == value;
}

bool operator==(
  std::string_view lhs,
  const diagnostic_code& rhs
) {
  return rhs == lhs;
}

diagnostic& diagnostic::with_note(
  diagnostic_note note
) {
  notes.push_back(std::move(note));
  return *this;
}

diagnostic_bundle_t make_error(
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
