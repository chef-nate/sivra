#include <sivra/canonicalizer/options.hpp>

namespace sivra::canonicalizer {

void options::enable_trait(
  ir::operation_trait trait
) {
  enabled_traits = enabled_traits | trait;
}

void options::disable_trait(
  ir::operation_trait trait
) {
  enabled_traits = enabled_traits & ~trait;
}

bool options::is_trait_enabled(
  ir::operation_trait trait
) const {
  return (enabled_traits & trait) == trait;
}

void options::enable_rule(
  rule canonicalization_rule
) {
  enabled_rules = enabled_rules | canonicalization_rule;
}

void options::disable_rule(
  rule canonicalization_rule
) {
  enabled_rules = enabled_rules & ~canonicalization_rule;
}

bool options::is_rule_enabled(
  rule canonicalization_rule
) const {
  return (enabled_rules & canonicalization_rule) == canonicalization_rule;
}

} // namespace sivra::canonicalizer
