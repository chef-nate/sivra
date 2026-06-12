#pragma once

#include "diagnostic.hpp"

#include <cstddef>
#include <string>

namespace sivra::core {

struct budget_limit {
  std::size_t maximum = 0;

  auto operator<=>(
    const budget_limit&
  ) const = default;
};

class budget_counter {
public:
  explicit budget_counter(
    std::size_t limit
  );

  explicit budget_counter(
    budget_limit limit
  );

  bool try_consume(
    std::size_t amount = 1
  );

  [[nodiscard]] std::size_t limit() const;
  [[nodiscard]] std::size_t consumed() const;
  [[nodiscard]] std::size_t remaining() const;
  [[nodiscard]] diagnostic exhaustion_diagnostic(
    std::string code,
    std::string message
  ) const;

private:
  std::size_t m_limit;
  std::size_t m_consumed = 0;
};

} // namespace sivra::core
