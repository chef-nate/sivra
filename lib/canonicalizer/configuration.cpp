#include <sivra/canonicalizer/configuration.hpp>

namespace sivra::canonicalizer {

configuration::configuration()
    : m_enabled_traits(
        ir::operation_trait::associative | ir::operation_trait::commutative |
        ir::operation_trait::idempotent
      ) {
  for (const auto& metadata : available_rules()) {
    if (metadata.enabled_by_default) {
      m_enabled_rules.insert(metadata.id);
    }
  }
}

core::result_t<void> configuration::enable_rule(
  const rule_id& rule
) {
  if (!is_known_rule(rule)) {
    return core::fail<void>(
      "canonicalizer.configuration.unknown_rule",
      "unknown canonicalizer rule: " + std::string(rule.key())
    );
  }
  m_enabled_rules.insert(rule);
  return {};
}

core::result_t<void> configuration::disable_rule(
  const rule_id& rule
) {
  if (!is_known_rule(rule)) {
    return core::fail<void>(
      "canonicalizer.configuration.unknown_rule",
      "unknown canonicalizer rule: " + std::string(rule.key())
    );
  }
  m_enabled_rules.erase(rule);
  return {};
}

bool configuration::is_rule_enabled(
  const rule_id& rule
) const {
  return m_enabled_rules.contains(rule);
}

void configuration::enable_trait(
  ir::operation_trait trait
) {
  m_enabled_traits = m_enabled_traits | trait;
}

void configuration::disable_trait(
  ir::operation_trait trait
) {
  m_enabled_traits = m_enabled_traits & ~trait;
}

bool configuration::is_trait_enabled(
  ir::operation_trait trait
) const {
  return (m_enabled_traits & trait) == trait;
}

const std::set<rule_id>& configuration::enabled_rules() const {
  return m_enabled_rules;
}

ir::operation_trait configuration::enabled_traits() const {
  return m_enabled_traits;
}

const canonicalization_limits& configuration::limits() const {
  return m_limits;
}

void configuration::set_limits(
  canonicalization_limits limits
) {
  m_limits = limits;
}

bool configuration::collect_trace() const {
  return m_collect_trace;
}

void configuration::set_collect_trace(
  bool enabled
) {
  m_collect_trace = enabled;
}

const equivalence_contract_id& configuration::contract() const {
  return algebraic_equivalence_contract();
}

core::result_t<void> configuration::validate() const {
  for (const auto& rule : m_enabled_rules) {
    if (!is_known_rule(rule)) {
      return core::fail<void>(
        "canonicalizer.configuration.unknown_rule",
        "unknown canonicalizer rule: " + std::string(rule.key())
      );
    }
  }
  return {};
}

} // namespace sivra::canonicalizer
