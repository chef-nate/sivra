#include <sivra/canonicalizer/rule.hpp>

#include <algorithm>
#include <array>
#include <utility>

namespace sivra::canonicalizer {

rule_id::rule_id(
  std::string key
)
    : m_key(std::move(key)) {
}

std::string_view rule_id::key() const {
  return m_key;
}

namespace builtin_rules {

#define SIVRA_CANONICALIZER_RULE(name, key, enabled_by_default, description)                       \
  const rule_id name(key);
#include <sivra/canonicalizer/rule.def>
#undef SIVRA_CANONICALIZER_RULE

} // namespace builtin_rules

namespace {

const std::array metadata{
#define SIVRA_CANONICALIZER_RULE(name, key, enabled_by_default, description)                       \
  rule_metadata{builtin_rules::name, #name, enabled_by_default, description},
#include <sivra/canonicalizer/rule.def>
#undef SIVRA_CANONICALIZER_RULE
};

} // namespace

std::span<const rule_metadata> available_rules() {
  return metadata;
}

bool is_known_rule(
  const rule_id& rule
) {
  return std::ranges::any_of(metadata, [&](const auto& entry) { return entry.id == rule; });
}

} // namespace sivra::canonicalizer
