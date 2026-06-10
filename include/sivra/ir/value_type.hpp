#pragma once

#include <compare>
#include <cstdint>

namespace sivra::ir {

enum class value_type_kind {
  unknown,
  scalar,
  vector,
};

enum class scalar_category {
  unknown,
  floating_point,
  signed_integer,
  unsigned_integer,
};

class value_type {
public:
  value_type() = default;

  static value_type unknown(
    std::uint32_t bit_width = 0
  );

  static value_type scalar(
    scalar_category category,
    std::uint32_t bit_width
  );

  static value_type f32();
  static value_type i32();

  static value_type vector(
    scalar_category element_category,
    std::uint32_t element_bit_width,
    std::uint32_t lane_count
  );

  value_type_kind kind() const;
  scalar_category category() const;
  std::uint32_t element_bit_width() const;
  std::uint32_t lane_count() const;
  std::uint32_t bit_width() const;

  auto operator<=>(
    const value_type&
  ) const = default;

private:
  value_type(
    value_type_kind kind,
    scalar_category category,
    std::uint32_t element_bit_width,
    std::uint32_t lane_count
  );

  value_type_kind m_kind = value_type_kind::unknown;
  scalar_category m_category = scalar_category::unknown;
  std::uint32_t m_element_bit_width = 0;
  std::uint32_t m_lane_count = 0;
};

} // namespace sivra::ir
