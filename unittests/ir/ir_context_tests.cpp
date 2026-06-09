#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

#include <type_traits>

static_assert(
  !std::is_copy_constructible_v<sivra::ir::ir_context>
);
static_assert(
  !std::is_move_constructible_v<sivra::ir::ir_context>
);
static_assert(
  !std::is_copy_assignable_v<sivra::ir::ir_context>
);
static_assert(
  !std::is_move_assignable_v<sivra::ir::ir_context>
);

TEST_CASE(
  "ir_context accessors return its owned registries"
) {
  sivra::ir::ir_context context;
  const auto& const_context = context;

  CHECK(&context.operations() == &const_context.operations());
  CHECK(&context.types() == &const_context.types());
}

TEST_CASE(
  "ir_context instances own distinct operations and types"
) {
  sivra::ir::ir_context lhs;
  sivra::ir::ir_context rhs;

  const auto lhs_operation = lhs.operations().register_operation("operation");
  const auto rhs_operation = rhs.operations().register_operation("operation");
  const auto& lhs_type = lhs.types().scalar(sivra::ir::scalar_type::f32);
  const auto& rhs_type = rhs.types().scalar(sivra::ir::scalar_type::f32);

  CHECK(lhs_operation == rhs_operation);
  CHECK(&lhs.operations().at(lhs_operation) != &rhs.operations().at(rhs_operation));
  CHECK(&lhs_type != &rhs_type);
  CHECK(&lhs_type.context() == &lhs.types());
  CHECK(&rhs_type.context() == &rhs.types());
}
