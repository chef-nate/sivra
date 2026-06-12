#include "rules.hpp"

#include <utility>
#include <variant>

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

} // namespace sivra::canonicalizer
