#include "rules.hpp"

#include "../detail/match.hpp"

#include <utility>
#include <vector>

namespace sivra::canonicalizer {

rewrite_result apply_bitwise_simplification(
  rewrite_context& context,
  const rewrite_subject& subject
) {
  const auto& key = context.operation(subject.operation).stable_key();
  if (!subject.attributes.empty()) {
    return no_match{};
  }

  if (key == ir::operation_key("bit_and_not") && subject.operands.size() == 2) {
    const auto all_bits_set = ir::operation_constant{ir::well_known_constant::all_bits_set};
    if (detail::constant_operand_matches(
          subject, context.node(subject.operands[0]), all_bits_set
        )) {
      auto zero = detail::make_integer_node(context, subject.result_type, 0);
      if (!zero.has_value()) {
        return invalid_rewrite{.diagnostic = std::move(zero.error().front())};
      }
      return replace_with{.replacement = *zero};
    }
    return no_match{};
  }

  if (key != ir::operation_key("bit_xor") || subject.operands.size() < 2) {
    return no_match{};
  }
  for (std::size_t lhs = 0; lhs < subject.operands.size(); ++lhs) {
    for (std::size_t rhs = lhs + 1; rhs < subject.operands.size(); ++rhs) {
      if (!context.structural().equal(
            context.graph(), subject.operands[lhs], context.graph(), subject.operands[rhs]
          )) {
        continue;
      }

      std::vector<ir::node_id> remaining;
      remaining.reserve(subject.operands.size() - 2);
      for (std::size_t index = 0; index < subject.operands.size(); ++index) {
        if (index != lhs && index != rhs) {
          remaining.push_back(subject.operands[index]);
        }
      }
      if (remaining.empty()) {
        auto zero = detail::make_integer_node(context, subject.result_type, 0);
        if (!zero.has_value()) {
          return invalid_rewrite{.diagnostic = std::move(zero.error().front())};
        }
        return replace_with{.replacement = *zero};
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
  }
  return no_match{};
}

} // namespace sivra::canonicalizer
