#pragma once

#include "id.hpp"
#include "phase.hpp"

#include <span>
#include <string_view>

namespace sivra::canonicalizer {

struct rule_metadata {
  rule_id id;
  std::string_view name;
  bool enabled_by_default;
  std::string_view description;
};

namespace builtin_rules {

#define SIVRA_CANONICALIZER_RULE(name, key, enabled_by_default, description)                       \
  extern const rule_id name;
#include "rule.def"
#undef SIVRA_CANONICALIZER_RULE

} // namespace builtin_rules

std::span<const rule_metadata> available_rules();
bool is_known_rule(
  const rule_id& rule
);

} // namespace sivra::canonicalizer
