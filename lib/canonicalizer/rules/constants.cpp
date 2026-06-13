#include "rules.hpp"

#include <utility>
#include <variant>
#include <vector>

namespace sivra::canonicalizer {

rewrite_result apply_constant_folding(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  auto evaluated = context.evaluate(subject);
  if (std::holds_alternative<no_evaluation>(evaluated)) {
    return no_match{};
  }
  if (auto* invalid = std::get_if<invalid_evaluation>(&evaluated)) {
    return invalid_rewrite{.diagnostic = std::move(invalid->diagnostic)};
  }

  auto built = context.make_constant(std::move(std::get<evaluated_constant>(evaluated).value));
  if (!built.has_value()) {
    return invalid_rewrite{.diagnostic = std::move(built.error().front())};
  }
  return replace_with{.replacement = *built};
}

rewrite_result apply_mixed_constant_aggregation(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& definition = context.operation(subject.operation);
  if (!definition.has_trait(ir::operation_trait::associative) || subject.operands.size() < 3 ||
      !subject.attributes.empty()) {
    return no_match{};
  }

  std::vector<ir::constant_value> constants;
  std::vector<ir::node_id> remaining;
  constants.reserve(subject.operands.size());
  remaining.reserve(subject.operands.size());
  for (const auto operand : subject.operands) {
    if (const auto* constant = context.node(operand).get_if_constant()) {
      constants.push_back(constant->value);
    } else {
      remaining.push_back(operand);
    }
  }
  if (constants.size() < 2 || remaining.empty()) {
    return no_match{};
  }

  auto evaluated = context.evaluate_constants(
    subject.operation, constants, subject.attributes, subject.result_type
  );
  if (auto* invalid = std::get_if<invalid_evaluation>(&evaluated)) {
    return invalid_rewrite{.diagnostic = std::move(invalid->diagnostic)};
  }
  auto* combined = std::get_if<evaluated_constant>(&evaluated);
  if (combined == nullptr) {
    return no_match{};
  }
  auto constant = context.make_constant(std::move(combined->value));
  if (!constant.has_value()) {
    return invalid_rewrite{.diagnostic = std::move(constant.error().front())};
  }
  remaining.push_back(*constant);
  return rebuild_expression{
    .operation = subject.operation,
    .operands = std::move(remaining),
    .attributes = subject.attributes,
    .result_type = subject.result_type,
  };
}

} // namespace sivra::canonicalizer
