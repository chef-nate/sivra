#include <sivra/ir/constant.hpp>
#include <sivra/ir/value_type.hpp>

#include <doctest/doctest.h>

#include <stdexcept>
#include <vector>

TEST_CASE(
  "value types use value semantics"
) {
  const auto lhs = sivra::ir::value_type::f32();
  const auto rhs = sivra::ir::value_type::scalar(sivra::ir::scalar_category::floating_point, 32);
  const auto vector =
    sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 4);

  CHECK(lhs == rhs);
  CHECK(lhs.kind() == sivra::ir::value_type_kind::scalar);
  CHECK(lhs.bit_width() == 32);
  CHECK(vector.kind() == sivra::ir::value_type_kind::vector);
  CHECK(vector.lane_count() == 4);
  CHECK(vector.bit_width() == 128);
}

TEST_CASE(
  "value types reject incomplete scalar and vector shapes"
) {
  CHECK_THROWS_AS(
    sivra::ir::value_type::scalar(sivra::ir::scalar_category::unknown, 32), std::invalid_argument
  );
  CHECK_THROWS_AS(
    sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 0),
    std::invalid_argument
  );
}

TEST_CASE(
  "constant values validate scalar variants and vector shape"
) {
  const auto f32 = sivra::ir::constant_value::scalar(
    sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(2.5F)
  );
  const auto vector_type =
    sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 2);
  const auto splat =
    sivra::ir::constant_value::splat(vector_type, sivra::ir::f32_constant::from_value(1.0F));
  const auto aggregate = sivra::ir::constant_value::aggregate(
    vector_type,
    {
      sivra::ir::f32_constant::from_value(1.0F),
      sivra::ir::f32_constant::from_value(2.0F),
    }
  );

  CHECK(std::get<sivra::ir::f32_constant>(f32.element(0)).value() == doctest::Approx(2.5F));
  CHECK(splat.is_splat());
  CHECK(splat.element_count() == 2);
  CHECK(!aggregate.is_splat());
  CHECK(aggregate.element_count() == 2);

  CHECK_THROWS_AS(
    sivra::ir::constant_value::scalar(
      sivra::ir::value_type::f32(), sivra::ir::i32_constant::from_value(1)
    ),
    std::invalid_argument
  );
  CHECK_THROWS_AS(
    sivra::ir::constant_value::aggregate(vector_type, {sivra::ir::f32_constant::from_value(1.0F)}),
    std::invalid_argument
  );
}
