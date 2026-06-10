#pragma once

#include <cstddef>

namespace sivra::core {

class budget_counter {
public:
  explicit budget_counter(
    std::size_t limit
  );

  bool try_consume(
    std::size_t amount = 1
  );

  std::size_t limit() const;
  std::size_t consumed() const;
  std::size_t remaining() const;

private:
  std::size_t m_limit;
  std::size_t m_consumed = 0;
};

} // namespace sivra::core
