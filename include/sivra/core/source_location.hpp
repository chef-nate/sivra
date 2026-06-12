#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>

namespace sivra::core {

class source_id {
public:
  constexpr source_id() = default;

  [[nodiscard]] static constexpr source_id from_index(
    std::uint32_t index
  ) {
    return source_id(index);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr bool is_valid() const { return m_index != invalid_index; }

  auto operator<=>(
    const source_id&
  ) const = default;

private:
  static constexpr std::uint32_t invalid_index = UINT32_MAX;

  explicit constexpr source_id(
    std::uint32_t index
  )
      : m_index(index) {}

  std::uint32_t m_index = invalid_index;
};

struct source_position {
  std::size_t byte_offset = 0;
  std::size_t line = 0;
  std::size_t column = 0;

  auto operator<=>(
    const source_position&
  ) const = default;
};

struct source_span {
  source_id source;
  source_position begin;
  source_position end;

  [[nodiscard]] bool is_valid() const;
  [[nodiscard]] bool contains(
    source_position position
  ) const;

  bool operator==(
    const source_span&
  ) const = default;
};

} // namespace sivra::core
