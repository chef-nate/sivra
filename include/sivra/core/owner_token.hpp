#pragma once

#include <compare>
#include <cstdint>

namespace sivra::core {

class owner_token {
public:
  constexpr owner_token() = default;

  static constexpr owner_token unsafe_from_value(
    std::uint64_t value
  ) {
    return owner_token(value);
  }

  constexpr std::uint64_t value() const { return m_value; }

  auto operator<=>(
    const owner_token&
  ) const = default;

private:
  explicit constexpr owner_token(
    std::uint64_t value
  )
      : m_value(value) {}

  std::uint64_t m_value = 0;
};

class owner_token_source {
public:
  static owner_token next();
};

} // namespace sivra::core
