#include "rules.hpp"

#include "../detail/match.hpp"

#include <utility>
#include <vector>

namespace sivra::canonicalizer {

rewrite_result apply_division_reciprocal_simplification(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& key = context.operation(subject.operation).stable_key();
  if (!subject.attributes.empty()) {
    return no_match{};
  }

  if (key == ir::operation_key("reciprocal") && subject.operands.size() == 1) {
    const auto& child = context.node(subject.operands.front());
    const auto* application = child.get_if_operation();
    if (application != nullptr && application->attributes.empty() &&
        application->operands.size() == 1 &&
        context.operation(application->operation).stable_key() == ir::operation_key("reciprocal")) {
      return replace_with{.replacement = application->operands.front()};
    }
    return no_match{};
  }

  const auto* multiply = context.catalogue().find(ir::operation_key("multiply"));
  const auto* reciprocal = context.catalogue().find(ir::operation_key("reciprocal"));
  if (multiply == nullptr || reciprocal == nullptr) {
    return no_match{};
  }

  if (key == ir::operation_key("divide") && subject.operands.size() == 2) {
    const auto numerator = subject.operands[0];
    const auto denominator = subject.operands[1];

    const auto* denominator_constant = context.node(denominator).get_if_constant();
    if (denominator_constant == nullptr) {
      return no_match{};
    }
    const ir::operation_constant zero{.element = ir::well_known_constant::zero};
    if (detail::constant_operand_matches(subject, context.node(denominator), zero)) {
      return no_match{};
    }
    auto inverse = context.rebuild(
      {
        .operation = reciprocal->id(),
        .operands = {denominator},
        .attributes = {},
        .result_type = subject.result_type,
      }
    );
    if (!inverse.has_value()) {
      return invalid_rewrite{.diagnostic = std::move(inverse.error().front())};
    }
    return rebuild_expression{
      .operation = multiply->id(),
      .operands = {numerator, *inverse},
      .attributes = {},
      .result_type = subject.result_type,
    };
  }

  return no_match{};
}

rewrite_result apply_square_simplification(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& key = context.operation(subject.operation).stable_key();
  if (!subject.attributes.empty()) {
    return no_match{};
  }

  if (key == ir::operation_key("multiply") && subject.operands.size() == 2 &&
      context.structural().equal(
        context.graph(), subject.operands[0], context.graph(), subject.operands[1]
      )) {
    const auto* square = context.catalogue().find(ir::operation_key("square"));
    if (square == nullptr) {
      return no_match{};
    }
    return rebuild_expression{
      .operation = square->id(),
      .operands = {subject.operands.front()},
      .attributes = {},
      .result_type = subject.result_type,
    };
  }

  return no_match{};
}

} // namespace sivra::canonicalizer
