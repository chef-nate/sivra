#include <sivra/ir/scalar_type.hpp>
#include <sivra/ir/type.hpp>

#include <doctest/doctest.h>

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
