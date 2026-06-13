#include "match.hpp"

#include <limits>
#include <utility>
#include <variant>

namespace {

const sivra::ir::constant_value* constant_value(
  const sivra::ir::expression_node& node
) {
  const auto* constant = node.get_if_constant();
  return constant == nullptr ? nullptr : &constant->value;
}

bool matches_well_known_constant(
  const sivra::ir::scalar_constant_t& actual,
  sivra::ir::well_known_constant expected
) {
  return std::visit(
    [expected](const auto& value) {
      switch (expected) {
      case sivra::ir::well_known_constant::zero:
        return value.value() == 0;
      case sivra::ir::well_known_constant::one:
        return value.value() == 1;
      case sivra::ir::well_known_constant::all_bits_set:
        return value.bits == std::numeric_limits<std::uint32_t>::max();
      }
      return false;
    },
    actual
  );
}

bool matches_operation_constant(
  const sivra::ir::scalar_constant_t& actual,
  const sivra::ir::operation_constant& expected,
  const sivra::ir::value_type& result_type
) {
  if (result_type.kind() != sivra::ir::value_type_kind::scalar &&
      result_type.kind() != sivra::ir::value_type_kind::vector) {
    return false;
  }
  if (const auto* well_known = std::get_if<sivra::ir::well_known_constant>(&expected.element)) {
    return matches_well_known_constant(actual, *well_known);
  }

  const auto& explicit_value = std::get<sivra::ir::scalar_constant_t>(expected.element);
  const bool matches_type = (result_type.category() == sivra::ir::scalar_category::floating_point &&
                             result_type.element_bit_width() == 32 &&
                             std::holds_alternative<sivra::ir::f32_constant>(explicit_value)) ||
                            (result_type.category() == sivra::ir::scalar_category::signed_integer &&
                             result_type.element_bit_width() == 32 &&
                             std::holds_alternative<sivra::ir::i32_constant>(explicit_value));
  return matches_type && actual == explicit_value;
}

} // namespace

namespace sivra::canonicalizer::detail {

bool constant_operand_matches(
  const rewrite_subject& subject,
  const ir::expression_node& child,
  const ir::operation_constant& expected
) {
  if (subject.result_type != child.result_type()) {
    return false;
  }
  const auto* constant = constant_value(child);
  if (constant == nullptr || constant->result_type() != subject.result_type) {
    return false;
  }
  for (std::size_t index = 0; index < constant->element_count(); ++index) {
    if (!matches_operation_constant(constant->element(index), expected, subject.result_type)) {
      return false;
    }
  }
  return true;
}

bool operand_matches(
  const rewrite_subject& subject,
  rewrite_context& context,
  std::size_t index,
  const std::optional<ir::operation_constant>& expected
) {
  return expected.has_value() &&
         constant_operand_matches(subject, context.node(subject.operands[index]), *expected);
}

core::result_t<ir::constant_value> integer_constant(
  const ir::value_type& type,
  std::int32_t value
) {
  ir::scalar_constant_t element;
  if (type.category() == ir::scalar_category::floating_point && type.element_bit_width() == 32) {
    element = ir::f32_constant::from_value(static_cast<float>(value));
  } else if (type.category() == ir::scalar_category::signed_integer &&
             type.element_bit_width() == 32) {
    element = ir::i32_constant::from_value(value);
  } else {
    return core::fail<ir::constant_value>(
      "canonicalizer.unsupported_constant_type",
      "algebraic rule cannot construct a constant for this result type"
    );
  }

  return type.kind() == ir::value_type_kind::scalar
           ? ir::constant_value::scalar(type, std::move(element))
           : ir::constant_value::splat(type, std::move(element));
}

core::result_t<ir::node_id> make_integer_node(
  rewrite_context& context,
  const ir::value_type& type,
  std::int32_t value
) {
  auto constant = integer_constant(type, value);
  if (!constant.has_value()) {
    return std::unexpected(std::move(constant.error()));
  }
  return context.make_constant(std::move(*constant));
}

} // namespace sivra::canonicalizer::detail
