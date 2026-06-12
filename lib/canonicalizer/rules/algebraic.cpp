#include "rules.hpp"

#include <sivra/ir/constant.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
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

[[nodiscard]] sivra::core::result_t<sivra::ir::constant_value> integer_constant(
  const sivra::ir::value_type& type,
  std::int32_t value
) {
  sivra::ir::scalar_constant_t element;
  if (type.category() == sivra::ir::scalar_category::floating_point &&
      type.element_bit_width() == 32) {
    element = sivra::ir::f32_constant::from_value(static_cast<float>(value));
  } else if (type.category() == sivra::ir::scalar_category::signed_integer &&
             type.element_bit_width() == 32) {
    element = sivra::ir::i32_constant::from_value(value);
  } else {
    return sivra::core::fail<sivra::ir::constant_value>(
      "canonicalizer.unsupported_constant_type",
      "algebraic rule cannot construct a constant for this result type"
    );
  }

  return type.kind() == sivra::ir::value_type_kind::scalar
           ? sivra::ir::constant_value::scalar(type, std::move(element))
           : sivra::ir::constant_value::splat(type, std::move(element));
}

struct coefficient_term {
  sivra::ir::constant_value coefficient;
  sivra::ir::node_id basis;
};

std::optional<coefficient_term> extract_coefficient_term(
  sivra::canonicalizer::rewrite_context& context,
  sivra::ir::node_id operand,
  const sivra::ir::value_type& result_type
) {
  const auto& node = context.node(operand);
  if (node.get_if_constant() != nullptr) {
    return std::nullopt;
  }
  const auto* application = node.get_if_operation();
  if (application != nullptr &&
      context.operation(application->operation).stable_key() ==
        sivra::ir::operation_key("multiply") &&
      node.result_type() == result_type && application->attributes.empty() &&
      application->operands.size() == 2) {
    const auto* lhs = context.node(application->operands[0]).get_if_constant();
    const auto* rhs = context.node(application->operands[1]).get_if_constant();
    if ((lhs == nullptr) == (rhs == nullptr)) {
      return std::nullopt;
    }
    return coefficient_term{
      .coefficient = lhs == nullptr ? rhs->value : lhs->value,
      .basis = lhs == nullptr ? application->operands[0] : application->operands[1],
    };
  }

  auto one = integer_constant(result_type, 1);
  if (!one.has_value()) {
    return std::nullopt;
  }
  return coefficient_term{
    .coefficient = std::move(*one),
    .basis = operand,
  };
}

} // namespace

namespace sivra::canonicalizer {

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
  return rebuild_expression{
    .operation = subject.operation,
    .operands = std::move(remaining),
    .attributes = subject.attributes,
    .result_type = subject.result_type,
  };
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

rewrite_result apply_same_operand_simplification(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  if (context.operation(subject.operation).stable_key() != ir::operation_key("subtract") ||
      subject.operands.size() != 2 ||
      !context.structural().equal(
        context.graph(), subject.operands[0], context.graph(), subject.operands[1]
      )) {
    return no_match{};
  }

  auto zero = integer_constant(subject.result_type, 0);
  if (!zero.has_value()) {
    return no_match{};
  }
  auto built = context.make_constant(std::move(*zero));
  if (!built.has_value()) {
    return invalid_rewrite{.diagnostic = std::move(built.error().front())};
  }
  return replace_with{.replacement = *built};
}

rewrite_result apply_coefficient_collection(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  if (context.operation(subject.operation).stable_key() != ir::operation_key("add") ||
      !subject.attributes.empty() || subject.operands.size() < 2) {
    return no_match{};
  }
  const auto* multiply = context.catalogue().find(ir::operation_key("multiply"));
  if (multiply == nullptr) {
    return no_match{};
  }

  for (std::size_t lhs_index = 0; lhs_index < subject.operands.size(); ++lhs_index) {
    auto lhs = extract_coefficient_term(context, subject.operands[lhs_index], subject.result_type);
    if (!lhs.has_value()) {
      continue;
    }
    for (std::size_t rhs_index = lhs_index + 1; rhs_index < subject.operands.size(); ++rhs_index) {
      auto rhs =
        extract_coefficient_term(context, subject.operands[rhs_index], subject.result_type);
      if (!rhs.has_value() ||
          !context.structural().equal(context.graph(), lhs->basis, context.graph(), rhs->basis)) {
        continue;
      }

      const std::array coefficients{lhs->coefficient, rhs->coefficient};
      auto evaluated = context.evaluate_constants(
        subject.operation, coefficients, subject.attributes, subject.result_type
      );
      if (auto* invalid = std::get_if<invalid_evaluation>(&evaluated)) {
        return invalid_rewrite{.diagnostic = std::move(invalid->diagnostic)};
      }
      auto* combined = std::get_if<evaluated_constant>(&evaluated);
      if (combined == nullptr) {
        return no_match{};
      }
      auto coefficient = context.make_constant(std::move(combined->value));
      if (!coefficient.has_value()) {
        return invalid_rewrite{.diagnostic = std::move(coefficient.error().front())};
      }
      auto term = context.rebuild(
        {
          .operation = multiply->id(),
          .operands = {*coefficient, lhs->basis},
          .attributes = {},
          .result_type = subject.result_type,
        }
      );
      if (!term.has_value()) {
        return invalid_rewrite{.diagnostic = std::move(term.error().front())};
      }

      std::vector<ir::node_id> operands;
      operands.reserve(subject.operands.size() - 1);
      for (std::size_t index = 0; index < subject.operands.size(); ++index) {
        if (index != lhs_index && index != rhs_index) {
          operands.push_back(subject.operands[index]);
        }
      }
      operands.push_back(*term);
      if (operands.size() == 1) {
        return replace_with{.replacement = operands.front()};
      }
      return rebuild_expression{
        .operation = subject.operation,
        .operands = std::move(operands),
        .attributes = subject.attributes,
        .result_type = subject.result_type,
      };
    }
  }
  return no_match{};
}

} // namespace sivra::canonicalizer
