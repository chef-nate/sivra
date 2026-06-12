#include <sivra/ir/operation_signature.hpp>

#include <doctest/doctest.h>

#include <array>

TEST_CASE(
  "operation signatures reject impossible arity ranges"
) {
  const sivra::ir::operation_signature signature{
    .arity = {.minimum = 3, .maximum = 2},
  };
  const auto result = signature.validate_definition();

  REQUIRE(!result.has_value());
  CHECK(result.error().front().code == "ir.signature.invalid_arity");
}

TEST_CASE(
  "operation signatures enforce arity and same-result operand types"
) {
  const sivra::ir::operation_signature signature{
    .arity = {.minimum = 2, .maximum = 2},
    .operand_types = sivra::ir::operand_type_constraint::same_as_result,
  };
  const std::array valid_types{sivra::ir::value_type::f32(), sivra::ir::value_type::f32()};
  const std::array invalid_types{sivra::ir::value_type::f32(), sivra::ir::value_type::i32()};

  CHECK(signature
          .validate_application(
            sivra::ir::value_type::f32(), valid_types, {}, sivra::ir::operation_attribute_schema{}
          )
          .has_value());

  const auto invalid_arity = signature.validate_application(
    sivra::ir::value_type::f32(),
    std::span<const sivra::ir::value_type>(valid_types).first(1),
    {},
    {}
  );
  REQUIRE(!invalid_arity.has_value());
  CHECK(invalid_arity.error().front().code == "ir.graph.invalid_arity");

  const auto invalid_type =
    signature.validate_application(sivra::ir::value_type::f32(), invalid_types, {}, {});
  REQUIRE(!invalid_type.has_value());
  CHECK(invalid_type.error().front().code == "ir.graph.type_mismatch");
}

TEST_CASE(
  "operation signatures can explicitly accept heterogeneous operands"
) {
  const sivra::ir::operation_signature signature{
    .arity = {.minimum = 2, .maximum = 2},
    .operand_types = sivra::ir::operand_type_constraint::any,
  };
  const std::array types{sivra::ir::value_type::f32(), sivra::ir::value_type::i32()};

  CHECK(signature.validate_application(sivra::ir::value_type::f32(), types, {}, {}).has_value());
}
