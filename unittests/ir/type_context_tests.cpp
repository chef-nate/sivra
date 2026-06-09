#include <sivra/ir/scalar_type.hpp>
#include <sivra/ir/type.hpp>

#include <doctest/doctest.h>

#include <stdexcept>
#include <type_traits>

static_assert(
  !std::is_copy_constructible_v<sivra::ir::type_context>
);
static_assert(
  !std::is_move_constructible_v<sivra::ir::type_context>
);
static_assert(
  !std::is_copy_assignable_v<sivra::ir::type_context>
);
static_assert(
  !std::is_move_assignable_v<sivra::ir::type_context>
);

TEST_CASE(
  "type_context reuses the unknown type"
) {
  sivra::ir::type_context types;

  const auto& lhs = types.unknown();
  const auto& rhs = types.unknown();

  CHECK(&lhs == &rhs);
  CHECK(lhs.kind() == sivra::ir::type_kind::unknown);
}

TEST_CASE(
  "type_context reuses scalar types"
) {
  sivra::ir::type_context types;

  const auto& lhs = types.scalar(sivra::ir::scalar_type::f32);
  const auto& rhs = types.scalar(sivra::ir::scalar_type::f32);
  const auto& other = types.scalar(sivra::ir::scalar_type::i32);

  CHECK(&lhs == &rhs);
  CHECK(&lhs != &other);
  CHECK(lhs.kind() == sivra::ir::type_kind::scalar);
  CHECK(lhs.scalar() == sivra::ir::scalar_type::f32);
}

TEST_CASE(
  "type_context reuses vector types by element identity and element count"
) {
  sivra::ir::type_context types;
  const auto& f32 = types.scalar(sivra::ir::scalar_type::f32);
  const auto& i32 = types.scalar(sivra::ir::scalar_type::i32);

  const auto& lhs = types.vector(f32, 4);
  const auto& rhs = types.vector(f32, 4);
  const auto& different_elements = types.vector(f32, 8);
  const auto& different_element_type = types.vector(i32, 4);

  CHECK(&lhs == &rhs);
  CHECK(&lhs != &different_elements);
  CHECK(&lhs != &different_element_type);
  CHECK(lhs.kind() == sivra::ir::type_kind::vector);
  CHECK(&lhs.element_type() == &f32);
  CHECK(lhs.elements() == 4);
}

TEST_CASE(
  "type_context reuses matrix types by element identity and shape"
) {
  sivra::ir::type_context types;
  const auto& f32 = types.scalar(sivra::ir::scalar_type::f32);
  const auto& i32 = types.scalar(sivra::ir::scalar_type::i32);

  const auto& lhs = types.matrix(f32, 4, 1);
  const auto& rhs = types.matrix(f32, 4, 1);
  const auto& transposed = types.matrix(f32, 1, 4);
  const auto& different_element_type = types.matrix(i32, 4, 1);

  CHECK(&lhs == &rhs);
  CHECK(&lhs != &transposed);
  CHECK(&lhs != &different_element_type);
  CHECK(lhs.kind() == sivra::ir::type_kind::matrix);
  CHECK(&lhs.element_type() == &f32);
  CHECK(lhs.rows() == 4);
  CHECK(lhs.columns() == 1);
}

TEST_CASE(
  "types report their owning type_context"
) {
  sivra::ir::type_context types;

  const auto& unknown = types.unknown();
  const auto& scalar = types.scalar(sivra::ir::scalar_type::f32);
  const auto& vector = types.vector(scalar, 4);
  const auto& matrix = types.matrix(scalar, 2, 2);

  CHECK(&unknown.context() == &types);
  CHECK(&scalar.context() == &types);
  CHECK(&vector.context() == &types);
  CHECK(&matrix.context() == &types);
}

TEST_CASE(
  "type_context rejects vector elements from another context"
) {
  sivra::ir::type_context types;
  sivra::ir::type_context foreign_types;
  const auto& foreign_f32 = foreign_types.scalar(sivra::ir::scalar_type::f32);

  CHECK_THROWS_AS(types.vector(foreign_f32, 4), std::invalid_argument);
}

TEST_CASE(
  "type_context rejects matrix elements from another context"
) {
  sivra::ir::type_context types;
  sivra::ir::type_context foreign_types;
  const auto& foreign_f32 = foreign_types.scalar(sivra::ir::scalar_type::f32);

  CHECK_THROWS_AS(types.matrix(foreign_f32, 2, 2), std::invalid_argument);
}

TEST_CASE(
  "type_context rejects non-scalar vector element types"
) {
  sivra::ir::type_context types;
  const auto& f32 = types.scalar(sivra::ir::scalar_type::f32);
  const auto& vector = types.vector(f32, 4);
  const auto& matrix = types.matrix(f32, 2, 2);

  CHECK_THROWS_AS(types.vector(types.unknown(), 4), std::invalid_argument);
  CHECK_THROWS_AS(types.vector(vector, 4), std::invalid_argument);
  CHECK_THROWS_AS(types.vector(matrix, 4), std::invalid_argument);
}

TEST_CASE(
  "type_context rejects non-scalar matrix element types"
) {
  sivra::ir::type_context types;
  const auto& f32 = types.scalar(sivra::ir::scalar_type::f32);
  const auto& vector = types.vector(f32, 4);
  const auto& matrix = types.matrix(f32, 2, 2);

  CHECK_THROWS_AS(types.matrix(types.unknown(), 2, 2), std::invalid_argument);
  CHECK_THROWS_AS(types.matrix(vector, 2, 2), std::invalid_argument);
  CHECK_THROWS_AS(types.matrix(matrix, 2, 2), std::invalid_argument);
}
