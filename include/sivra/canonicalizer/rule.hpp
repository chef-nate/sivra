#pragma once

#include <compare>
#include <span>
#include <string>
#include <string_view>

namespace sivra::canonicalizer {

class rule_id {
public:
  explicit rule_id(
    std::string key
  );

  [[nodiscard]] std::string_view key() const;

  auto operator<=>(
    const rule_id&
  ) const = default;

private:
  std::string m_key;
};

enum class pass_phase {
  validation,
  local_simplification,
  shape_normalization,
  algebraic_collection,
  domain_normalization,
  cleanup,
  verification,
};

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
