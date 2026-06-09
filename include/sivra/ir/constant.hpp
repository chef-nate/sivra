#pragma once

#include "type.hpp"

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace sivra::ir {

/**
 * @struct f32_constant
 * @brief Stores the exact bit representation of an f32 constant.
 */
struct f32_constant {
  std::uint32_t bits;

  static f32_constant from_value(
    float value
  );

  float value() const;

  bool operator==(
    const f32_constant&
  ) const = default;
};

/**
 * @struct i32_constant
 * @brief Stores the exact bit representation of an i32 constant.
 */
struct i32_constant {
  std::uint32_t bits;

  static i32_constant from_value(
    std::int32_t value
  );

  std::int32_t value() const;

  bool operator==(
    const i32_constant&
  ) const = default;
};

/**
 * @brief Exact value of one scalar constant element.
 */
using scalar_constant_t = std::variant<f32_constant, i32_constant>;

/**
 * @class constant_value
 * @brief Represents a typed scalar, splat, or aggregate constant.
 */
class constant_value {
public:
  static constant_value scalar(
    const scalar_type_def& result_type,
    scalar_constant_t value
  );

  static constant_value splat(
    const type& result_type,
    scalar_constant_t element
  );

  static constant_value aggregate(
    const type& result_type,
    std::vector<scalar_constant_t> elements
  );

  const type& result_type() const;
  std::size_t element_count() const;
  bool is_splat() const;

  const scalar_constant_t& element(
    std::size_t index
  ) const;

private:
  constant_value(
    const type& result_type,
    std::variant<
      scalar_constant_t,
      std::vector<scalar_constant_t>
    > storage
  );

  const type* m_result_type;
  std::variant<scalar_constant_t, std::vector<scalar_constant_t>> m_storage;
};

} // namespace sivra::ir
