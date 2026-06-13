#include "rules.hpp"

#include <algorithm>
#include <vector>

namespace sivra::canonicalizer {

rewrite_result apply_associative_flattening(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  constexpr std::size_t maximum_flattened_operands = 64;

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
        child.result_type() != subject.result_type ||
        application->attributes != subject.attributes || application->operands.size() < 2 ||
        flattened.size() + application->operands.size() > maximum_flattened_operands) {
      flattened.push_back(operand);
      continue;
    }
    flattened.insert(flattened.end(), application->operands.begin(), application->operands.end());
    changed = true;
  }

  if (!changed) {
    return no_match{};
  }
  return rebuild_expression{
    .operation = subject.operation,
    .operands = std::move(flattened),
    .attributes = subject.attributes,
    .result_type = subject.result_type,
  };
}

rewrite_result apply_commutative_ordering(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& definition = context.operation(subject.operation);
  if (!context.is_trait_enabled(ir::operation_trait::commutative) ||
      !definition.has_trait(ir::operation_trait::commutative) || subject.operands.size() < 2) {
    return no_match{};
  }

  auto ordered = subject.operands;
  std::ranges::stable_sort(ordered, [&](ir::node_id lhs, ir::node_id rhs) {
    return context.structural().compare(context.graph(), lhs, context.graph(), rhs) ==
           std::strong_ordering::less;
  });
  if (ordered == subject.operands) {
    return no_match{};
  }
  return rebuild_expression{
    .operation = subject.operation,
    .operands = std::move(ordered),
    .attributes = subject.attributes,
    .result_type = subject.result_type,
  };
}

rewrite_result apply_idempotent_deduplication(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& definition = context.operation(subject.operation);
  if (!context.is_trait_enabled(ir::operation_trait::idempotent) ||
      !definition.has_trait(ir::operation_trait::idempotent) || subject.operands.size() < 2) {
    return no_match{};
  }

  std::vector<ir::node_id> unique;
  unique.reserve(subject.operands.size());
  for (const auto operand : subject.operands) {
    const auto duplicate = std::ranges::any_of(unique, [&](ir::node_id existing) {
      return context.structural().equal(context.graph(), existing, context.graph(), operand);
    });
    if (!duplicate) {
      unique.push_back(operand);
    }
  }
  if (unique.size() == subject.operands.size()) {
    return no_match{};
  }
  if (unique.size() == 1) {
    return replace_with{.replacement = unique.front()};
  }
  return rebuild_expression{
    .operation = subject.operation,
    .operands = std::move(unique),
    .attributes = subject.attributes,
    .result_type = subject.result_type,
  };
}

} // namespace sivra::canonicalizer
