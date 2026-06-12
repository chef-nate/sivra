#include <sivra/ir/value_type.hpp>

#include <cstddef>
#include <limits>

namespace sivra::ir {

value_type value_type::unknown(
  std::uint32_t bit_width
) {
  return value_type(value_type_kind::unknown, scalar_category::unknown, bit_width, 0);
}

core::result_t<value_type> value_type::scalar(
  scalar_category category,
  std::uint32_t bit_width
) {
  if (category == scalar_category::unknown || bit_width == 0) {
    return core::fail<value_type>(
      "ir.value_type.invalid_scalar", "scalar value_type requires a category and non-zero width"
    );
  }
  return value_type(value_type_kind::scalar, category, bit_width, 1);
}

value_type value_type::f32() {
  return value_type(value_type_kind::scalar, scalar_category::floating_point, 32, 1);
}

value_type value_type::i32() {
  return value_type(value_type_kind::scalar, scalar_category::signed_integer, 32, 1);
}

core::result_t<value_type> value_type::vector(
  scalar_category element_category,
  std::uint32_t element_bit_width,
  std::uint32_t lane_count
) {
  if (element_category == scalar_category::unknown || element_bit_width == 0 || lane_count == 0) {
    return core::fail<value_type>(
      "ir.value_type.invalid_vector",
      "vector value_type requires an element type and non-zero lanes"
    );
  }
  if (lane_count > std::numeric_limits<std::uint32_t>::max() / element_bit_width) {
    return core::fail<value_type>(
      "ir.value_type.invalid_vector", "vector value_type bit width exceeds uint32_t"
    );
  }
  return value_type(value_type_kind::vector, element_category, element_bit_width, lane_count);
}

value_type_kind value_type::kind() const {
  return m_kind;
}

scalar_category value_type::category() const {
  return m_category;
}

std::uint32_t value_type::element_bit_width() const {
  return m_element_bit_width;
}

std::uint32_t value_type::lane_count() const {
  return m_lane_count;
}

std::uint32_t value_type::bit_width() const {
  return m_kind == value_type_kind::vector ? m_element_bit_width * m_lane_count
                                           : m_element_bit_width;
}

core::result_t<void> value_type::validate() const {
  switch (m_kind) {
  case value_type_kind::unknown:
    if (m_category == scalar_category::unknown && m_lane_count == 0) {
      return {};
    }
    break;
  case value_type_kind::scalar:
    if (m_category != scalar_category::unknown && m_element_bit_width != 0 && m_lane_count == 1) {
      return {};
    }
    break;
  case value_type_kind::vector:
    if (m_category != scalar_category::unknown && m_element_bit_width != 0 && m_lane_count != 0 &&
        m_lane_count <= std::numeric_limits<std::uint32_t>::max() / m_element_bit_width) {
      return {};
    }
    break;
  }
  return core::fail<void>("ir.value_type.invalid", "value type has an invalid shape");
}

value_type::value_type(
  value_type_kind kind,
  scalar_category category,
  std::uint32_t element_bit_width,
  std::uint32_t lane_count
)
    : m_kind(kind),
      m_category(category),
      m_element_bit_width(element_bit_width),
      m_lane_count(lane_count) {
}

} // namespace sivra::ir
