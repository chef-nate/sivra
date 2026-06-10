#include <sivra/core/budget.hpp>

#include <algorithm>

namespace sivra::core {

budget_counter::budget_counter(
  std::size_t limit
)
    : m_limit(limit) {
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

} // namespace sivra::core
