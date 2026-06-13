#include "rules.hpp"

#include "../detail/match.hpp"

#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace {

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

  auto one = sivra::canonicalizer::detail::integer_constant(result_type, 1);
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

rewrite_result apply_copy_elimination(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  if (context.operation(subject.operation).stable_key() != ir::operation_key("copy") ||
      !subject.attributes.empty() || subject.operands.size() != 1) {
    return no_match{};
  }
  return replace_with{.replacement = subject.operands.front()};
}

rewrite_result apply_identity_elimination(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& semantics = context.operation(subject.operation).semantics();
  if (!semantics.identity.has_value() && !semantics.left_identity.has_value() &&
      !semantics.right_identity.has_value()) {
    return no_match{};
  }

  std::optional<ir::node_id> first_identity;
  std::vector<ir::node_id> remaining;
  remaining.reserve(subject.operands.size());
  for (std::size_t index = 0; index < subject.operands.size(); ++index) {
    const auto operand = subject.operands[index];
    const bool matches =
      detail::operand_matches(subject, context, index, semantics.identity) ||
      (index == 0 && detail::operand_matches(subject, context, index, semantics.left_identity)) ||
      (index + 1 == subject.operands.size() &&
       detail::operand_matches(subject, context, index, semantics.right_identity));
    if (matches) {
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
  const auto& semantics = context.operation(subject.operation).semantics();
  if (!semantics.annihilator.has_value() && !semantics.left_annihilator.has_value() &&
      !semantics.right_annihilator.has_value()) {
    return no_match{};
  }

  for (std::size_t index = 0; index < subject.operands.size(); ++index) {
    const bool matches =
      detail::operand_matches(subject, context, index, semantics.annihilator) ||
      (index == 0 &&
       detail::operand_matches(subject, context, index, semantics.left_annihilator)) ||
      (index + 1 == subject.operands.size() &&
       detail::operand_matches(subject, context, index, semantics.right_annihilator));
    if (matches) {
      return replace_with{.replacement = subject.operands[index]};
    }
  }
  return no_match{};
}

rewrite_result apply_same_operand_simplification(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  if (subject.operands.size() != 2 ||
      !context.structural().equal(
        context.graph(), subject.operands[0], context.graph(), subject.operands[1]
      )) {
    return no_match{};
  }

  const auto& key = context.operation(subject.operation).stable_key();
  std::optional<std::int32_t> replacement;
  if (key == ir::operation_key("subtract") || key == ir::operation_key("bit_xor") ||
      key == ir::operation_key("bit_and_not")) {
    replacement = 0;
  } else {
    return no_match{};
  }

  auto built = detail::make_integer_node(context, subject.result_type, *replacement);
  if (!built.has_value()) {
    return invalid_rewrite{.diagnostic = std::move(built.error().front())};
  }
  return replace_with{.replacement = *built};
}

rewrite_result apply_subtraction_normalization(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  if (context.operation(subject.operation).stable_key() != ir::operation_key("subtract") ||
      !subject.attributes.empty() || subject.operands.size() != 2) {
    return no_match{};
  }
  const auto* add = context.catalogue().find(ir::operation_key("add"));
  const auto* multiply = context.catalogue().find(ir::operation_key("multiply"));
  if (add == nullptr || multiply == nullptr) {
    return no_match{};
  }

  auto negative_one = detail::make_integer_node(context, subject.result_type, -1);
  if (!negative_one.has_value()) {
    return invalid_rewrite{.diagnostic = std::move(negative_one.error().front())};
  }
  auto negative_rhs = context.rebuild(
    {
      .operation = multiply->id(),
      .operands = {*negative_one, subject.operands[1]},
      .attributes = {},
      .result_type = subject.result_type,
    }
  );
  if (!negative_rhs.has_value()) {
    return invalid_rewrite{.diagnostic = std::move(negative_rhs.error().front())};
  }
  return rebuild_expression{
    .operation = add->id(),
    .operands = {subject.operands[0], *negative_rhs},
    .attributes = {},
    .result_type = subject.result_type,
  };
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
