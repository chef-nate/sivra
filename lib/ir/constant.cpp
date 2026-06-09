#include <sivra/ir/constant.hpp>

#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>
#include <variant>

namespace {

const sivra::ir::scalar_type_def& aggregate_element_type(
  const sivra::ir::type& result_type
) {
  const sivra::ir::type* element_type = nullptr;

  switch (result_type.kind()) {
  case sivra::ir::type_kind::vector:
    element_type = &static_cast<const sivra::ir::vector_type_def&>(result_type).element_type();
    break;

  case sivra::ir::type_kind::matrix:
    element_type = &static_cast<const sivra::ir::matrix_type_def&>(result_type).element_type();
    break;

  case sivra::ir::type_kind::unknown:
  case sivra::ir::type_kind::scalar:
    throw std::invalid_argument("constant aggregate requires a vector or matrix result type");
  }

  if (element_type->kind() != sivra::ir::type_kind::scalar) {
    throw std::invalid_argument("constant aggregate element type must be scalar");
  }

  return static_cast<const sivra::ir::scalar_type_def&>(*element_type);
}

std::size_t aggregate_element_count(
  const sivra::ir::type& result_type
) {
  switch (result_type.kind()) {
  case sivra::ir::type_kind::vector:
    return static_cast<const sivra::ir::vector_type_def&>(result_type).elements();

  case sivra::ir::type_kind::matrix: {
    const auto& matrix = static_cast<const sivra::ir::matrix_type_def&>(result_type);
    const auto rows = static_cast<std::size_t>(matrix.rows());
    const auto columns = static_cast<std::size_t>(matrix.columns());

    if (columns != 0 && rows > std::numeric_limits<std::size_t>::max() / columns) {
      throw std::length_error("matrix constant element count exceeds size_t");
    }

    return rows * columns;
  }

  case sivra::ir::type_kind::unknown:
  case sivra::ir::type_kind::scalar:
    throw std::invalid_argument("constant aggregate requires a vector or matrix result type");
  }

  throw std::invalid_argument("unsupported constant result type");
}

bool matches_scalar_type(
  const sivra::ir::scalar_constant_t& value,
  sivra::ir::scalar_type expected
) {
  switch (expected) {
  case sivra::ir::scalar_type::f32:
    return std::holds_alternative<sivra::ir::f32_constant>(value);

  case sivra::ir::scalar_type::i32:
    return std::holds_alternative<sivra::ir::i32_constant>(value);

  case sivra::ir::scalar_type::unknown:
    return false;
  }

  return false;
}

void validate_element_type(
  const sivra::ir::scalar_constant_t& value,
  const sivra::ir::scalar_type_def& expected
) {
  if (!matches_scalar_type(value, expected.scalar())) {
    throw std::invalid_argument("constant element does not match the result scalar type");
  }
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

constant_value constant_value::scalar(
  const scalar_type_def& result_type,
  scalar_constant_t value
) {
  validate_element_type(value, result_type);
  return constant_value(result_type, std::move(value));
}

constant_value constant_value::splat(
  const type& result_type,
  scalar_constant_t element
) {
  validate_element_type(element, aggregate_element_type(result_type));
  return constant_value(result_type, std::move(element));
}

constant_value constant_value::aggregate(
  const type& result_type,
  std::vector<scalar_constant_t> elements
) {
  const auto& element_type = aggregate_element_type(result_type);
  if (elements.size() != aggregate_element_count(result_type)) {
    throw std::invalid_argument("constant aggregate element count does not match result type");
  }

  for (const auto& element : elements) {
    validate_element_type(element, element_type);
  }

  return constant_value(result_type, std::move(elements));
}

const type& constant_value::result_type() const {
  return *m_result_type;
}

std::size_t constant_value::element_count() const {
  if (m_result_type->kind() == type_kind::scalar) {
    return 1;
  }

  return aggregate_element_count(*m_result_type);
}

bool constant_value::is_splat() const {
  return m_result_type->kind() != type_kind::scalar &&
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
  const type& result_type,
  std::variant<
    scalar_constant_t,
    std::vector<scalar_constant_t>
  > storage
)
    : m_result_type(&result_type),
      m_storage(std::move(storage)) {
}

} // namespace sivra::ir
