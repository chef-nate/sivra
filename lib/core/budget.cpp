#include <sivra/core/budget.hpp>

#include <algorithm>
#include <utility>

namespace sivra::core {

budget_counter::budget_counter(
  std::size_t limit
)
    : m_limit(limit) {
}

budget_counter::budget_counter(
  budget_limit limit
)
    : budget_counter(limit.maximum) {
}

bool budget_counter::try_consume(
  std::size_t amount
) {
  if (amount > remaining()) {
    return false;
  }
  m_consumed += amount;
  return true;
}

std::size_t budget_counter::limit() const {
  return m_limit;
}

std::size_t budget_counter::consumed() const {
  return m_consumed;
}

std::size_t budget_counter::remaining() const {
  return m_limit - std::min(m_limit, m_consumed);
}

diagnostic budget_counter::exhaustion_diagnostic(
  std::string code,
  std::string message
) const {
  return {
    .code = std::move(code),
    .severity = diagnostic_severity::error,
    .message = std::move(message),
    .notes =
      {
        diagnostic_note{
          .message = "consumed " + std::to_string(m_consumed) + " of " + std::to_string(m_limit),
        },
      },
  };
}

} // namespace sivra::core
