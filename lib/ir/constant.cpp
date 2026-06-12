#include <sivra/ir/constant.hpp>

#include <bit>
#include <stdexcept>
#include <utility>

namespace {

bool matches_type(
  const sivra::ir::scalar_constant_t& value,
  const sivra::ir::value_type& type
) {
  if (type.category() == sivra::ir::scalar_category::floating_point &&
      type.element_bit_width() == 32) {
    return std::holds_alternative<sivra::ir::f32_constant>(value);
  }
  if (type.category() == sivra::ir::scalar_category::signed_integer &&
      type.element_bit_width() == 32) {
    return std::holds_alternative<sivra::ir::i32_constant>(value);
  }
  return false;
}

[[nodiscard]] sivra::core::result_t<void> validate_element(
  const sivra::ir::scalar_constant_t& value,
  const sivra::ir::value_type& type
) {
  if (!matches_type(value, type)) {
    return sivra::core::fail<void>(
      "ir.constant.type_mismatch", "constant element does not match result type"
    );
  }
  return {};
}

} // namespace

namespace sivra::ir {

f32_constant f32_constant::from_value(
  float value
) {
  return f32_constant{.bits = std::bit_cast<std::uint32_t>(value)};
}

float f32_constant::value() const {
  return std::bit_cast<float>(bits);
}

i32_constant i32_constant::from_value(
  std::int32_t value
) {
  return i32_constant{.bits = std::bit_cast<std::uint32_t>(value)};
}

std::int32_t i32_constant::value() const {
  return std::bit_cast<std::int32_t>(bits);
}

core::result_t<constant_value> constant_value::scalar(
  value_type result_type,
  scalar_constant_t value
) {
  if (result_type.kind() != value_type_kind::scalar) {
    return core::fail<constant_value>(
      "ir.constant.invalid_shape", "scalar constant requires scalar result type"
    );
  }
  if (auto validated = validate_element(value, result_type); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }
  return constant_value(std::move(result_type), std::move(value));
}

core::result_t<constant_value> constant_value::splat(
  value_type result_type,
  scalar_constant_t element
) {
  if (result_type.kind() != value_type_kind::vector) {
    return core::fail<constant_value>(
      "ir.constant.invalid_shape", "splat constant requires vector result type"
    );
  }
  if (auto validated = validate_element(element, result_type); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }
  return constant_value(std::move(result_type), std::move(element));
}

core::result_t<constant_value> constant_value::aggregate(
  value_type result_type,
  std::vector<scalar_constant_t> elements
) {
  if (result_type.kind() != value_type_kind::vector) {
    return core::fail<constant_value>(
      "ir.constant.invalid_shape", "aggregate constant requires vector result type"
    );
  }
  if (elements.size() != result_type.lane_count()) {
    return core::fail<constant_value>(
      "ir.constant.invalid_shape", "constant element count does not match result type"
    );
  }
  for (const auto& element : elements) {
    if (auto validated = validate_element(element, result_type); !validated.has_value()) {
      return std::unexpected(std::move(validated.error()));
    }
  }
  return constant_value(std::move(result_type), std::move(elements));
}

const value_type& constant_value::result_type() const {
  return m_result_type;
}

std::size_t constant_value::element_count() const {
  return m_result_type.kind() == value_type_kind::vector ? m_result_type.lane_count() : 1;
}

bool constant_value::is_splat() const {
  return m_result_type.kind() == value_type_kind::vector &&
         std::holds_alternative<scalar_constant_t>(m_storage);
}

const scalar_constant_t& constant_value::element(
  std::size_t index
) const {
  if (index >= element_count()) {
    throw std::out_of_range("constant element index out of range");
  }
  if (const auto* repeated = std::get_if<scalar_constant_t>(&m_storage)) {
    return *repeated;
  }
  return std::get<std::vector<scalar_constant_t>>(m_storage).at(index);
}

constant_value::constant_value(
  value_type result_type,
  std::variant<
    scalar_constant_t,
    std::vector<scalar_constant_t>
  > storage
)
    : m_result_type(std::move(result_type)),
      m_storage(std::move(storage)) {
}

} // namespace sivra::ir
