#pragma once

#include <sivra/core/hash.hpp>
#include <sivra/core/result.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

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

  static core::result_t<value_type> scalar(
    scalar_category category,
    std::uint32_t bit_width
  );

  static value_type f32();
  static value_type i32();

  static core::result_t<value_type> vector(
    scalar_category element_category,
    std::uint32_t element_bit_width,
    std::uint32_t lane_count
  );

  [[nodiscard]] value_type_kind kind() const;
  [[nodiscard]] scalar_category category() const;
  [[nodiscard]] std::uint32_t element_bit_width() const;
  [[nodiscard]] std::uint32_t lane_count() const;
  [[nodiscard]] std::uint32_t bit_width() const;
  [[nodiscard]] core::result_t<void> validate() const;

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

template <>
struct std::hash<sivra::ir::value_type> {
  std::size_t operator()(
    const sivra::ir::value_type& value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(
      seed, value.kind(), value.category(), value.element_bit_width(), value.lane_count()
    );
    return seed;
  }
};
