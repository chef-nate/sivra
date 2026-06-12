#include "rules.hpp"

#include <sivra/ir/constant.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <variant>
#include <vector>

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

bool constant_operand_matches(
  const sivra::canonicalizer::rewrite_subject& subject,
  const sivra::ir::expression_node& child,
  const sivra::ir::operation_constant& expected
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

} // namespace

namespace sivra::canonicalizer {

rewrite_result apply_associative_flattening(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& definition = context.operation(subject.operation);
  if (!context.is_trait_enabled(ir::operation_trait::associative) ||
      !definition.has_trait(ir::operation_trait::associative) || subject.operands.size() < 2) {
    return no_match{};
  }

  std::vector<ir::node_id> flattened;
  flattened.reserve(subject.operands.size());
  bool changed = false;

  for (const auto operand : subject.operands) {
    const auto& child = context.node(operand);
    const auto* application = child.get_if_operation();
    if (application == nullptr || application->operation != subject.operation ||
        child.result_type() != subject.result_type || application->operands.size() < 2) {
      flattened.push_back(operand);
      continue;
    }
    flattened.insert(flattened.end(), application->operands.begin(), application->operands.end());
    changed = true;
  }

  if (!changed) {
    return no_match{};
  }
  return rebuild_expression{.operands = std::move(flattened)};
}

rewrite_result apply_identity_elimination(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& identity = context.operation(subject.operation).semantics().identity;
  if (!identity.has_value()) {
    return no_match{};
  }

  std::optional<ir::node_id> first_identity;
  std::vector<ir::node_id> remaining;
  remaining.reserve(subject.operands.size());
  for (const auto operand : subject.operands) {
    if (constant_operand_matches(subject, context.node(operand), *identity)) {
      if (!first_identity.has_value()) {
        first_identity = operand;
      }
    } else {
      remaining.push_back(operand);
    }
  }

  if (!first_identity.has_value()) {
    return no_match{};
  }
  if (remaining.empty()) {
    return replace_with{.replacement = *first_identity};
  }
  if (remaining.size() == 1) {
    return replace_with{.replacement = remaining.front()};
  }
  return rebuild_expression{.operands = std::move(remaining)};
}

rewrite_result apply_annihilator_collapse(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& annihilator = context.operation(subject.operation).semantics().annihilator;
  if (!annihilator.has_value()) {
    return no_match{};
  }

  for (const auto operand : subject.operands) {
    if (constant_operand_matches(subject, context.node(operand), *annihilator)) {
      return replace_with{.replacement = operand};
    }
  }
  return no_match{};
}

} // namespace sivra::canonicalizer
