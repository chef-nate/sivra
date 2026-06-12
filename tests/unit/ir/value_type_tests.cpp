#include <sivra/ir/constant.hpp>
#include <sivra/ir/value_type.hpp>

#include <doctest/doctest.h>

#include <bit>
#include <cstdint>
#include <limits>
#include <vector>

TEST_CASE(
  "value types use value semantics"
) {
  const auto lhs = sivra::ir::value_type::f32();
  const auto rhs = sivra::ir::value_type::scalar(sivra::ir::scalar_category::floating_point, 32);
  const auto vector =
    sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 4);

  REQUIRE(rhs.has_value());
  REQUIRE(vector.has_value());
  CHECK(lhs == *rhs);
  CHECK(lhs.kind() == sivra::ir::value_type_kind::scalar);
  CHECK(lhs.bit_width() == 32);
  CHECK(vector->kind() == sivra::ir::value_type_kind::vector);
  CHECK(vector->lane_count() == 4);
  CHECK(vector->bit_width() == 128);
  CHECK(lhs.validate().has_value());
  CHECK(std::hash<sivra::ir::value_type>{}(lhs) == std::hash<sivra::ir::value_type>{}(*rhs));
}

TEST_CASE(
  "value types return diagnostics for incomplete scalar and vector shapes"
) {
  const auto scalar = sivra::ir::value_type::scalar(sivra::ir::scalar_category::unknown, 32);
  const auto vector =
    sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 0);

  REQUIRE(!scalar.has_value());
  CHECK(scalar.error().front().code == "ir.value_type.invalid_scalar");
  REQUIRE(!vector.has_value());
  CHECK(vector.error().front().code == "ir.value_type.invalid_vector");
}

TEST_CASE(
  "exact scalar constants preserve NaN, signed-zero, and all-bits-set representations"
) {
  const sivra::ir::f32_constant nan{.bits = 0x7fc01234U};
  const sivra::ir::f32_constant positive_zero{.bits = 0x00000000U};
  const sivra::ir::f32_constant negative_zero{.bits = 0x80000000U};
  const sivra::ir::i32_constant all_bits_set{.bits = 0xffffffffU};

  CHECK(std::bit_cast<std::uint32_t>(nan.value()) == 0x7fc01234U);
  CHECK(positive_zero != negative_zero);
  CHECK(std::bit_cast<std::uint32_t>(negative_zero.value()) == 0x80000000U);
  CHECK(all_bits_set.value() == -1);
}

TEST_CASE(
  "constant values validate scalar variants and vector shape"
) {
  const auto f32 = sivra::ir::constant_value::scalar(
    sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(2.5F)
  );
  const auto vector_type =
    sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 2);
  REQUIRE(f32.has_value());
  REQUIRE(vector_type.has_value());
  const auto splat =
    sivra::ir::constant_value::splat(*vector_type, sivra::ir::f32_constant::from_value(1.0F));
  const auto aggregate = sivra::ir::constant_value::aggregate(
    *vector_type,
    {
      sivra::ir::f32_constant::from_value(1.0F),
      sivra::ir::f32_constant::from_value(2.0F),
    }
  );
  REQUIRE(splat.has_value());
  REQUIRE(aggregate.has_value());

  CHECK(std::get<sivra::ir::f32_constant>(f32->element(0)).value() == doctest::Approx(2.5F));
  CHECK(splat->is_splat());
  CHECK(splat->element_count() == 2);
  CHECK(!aggregate->is_splat());
  CHECK(aggregate->element_count() == 2);

  const auto wrong_type = sivra::ir::constant_value::scalar(
    sivra::ir::value_type::f32(), sivra::ir::i32_constant::from_value(1)
  );
  const auto wrong_lanes =
    sivra::ir::constant_value::aggregate(*vector_type, {sivra::ir::f32_constant::from_value(1.0F)});
  REQUIRE(!wrong_type.has_value());
  CHECK(wrong_type.error().front().code == "ir.constant.type_mismatch");
  REQUIRE(!wrong_lanes.has_value());
  CHECK(wrong_lanes.error().front().code == "ir.constant.invalid_shape");
}
