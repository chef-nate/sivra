#pragma once

#include "equivalence_contract.hpp"
#include "rule.hpp"

#include <sivra/core/result.hpp>
#include <sivra/ir/operation.hpp>

#include <cstddef>
#include <set>

namespace sivra::canonicalizer {

struct canonicalization_limits {
  std::size_t maximum_imported_nodes = 1'000'000;
  std::size_t maximum_output_nodes = 1'000'000;
  std::size_t maximum_worklist_steps = 1'000'000;
  std::size_t maximum_rewrites = 100'000;
};

class configuration {
public:
  configuration();

  core::result_t<void> enable_rule(
    const rule_id& rule
  );

  core::result_t<void> disable_rule(
    const rule_id& rule
  );

  [[nodiscard]] bool is_rule_enabled(
    const rule_id& rule
  ) const;

  void enable_trait(
    ir::operation_trait trait
  );

  void disable_trait(
    ir::operation_trait trait
  );

  [[nodiscard]] bool is_trait_enabled(
    ir::operation_trait trait
  ) const;

  [[nodiscard]] const std::set<rule_id>& enabled_rules() const;
  [[nodiscard]] ir::operation_trait enabled_traits() const;

  [[nodiscard]] const canonicalization_limits& limits() const;
  void set_limits(
    canonicalization_limits limits
  );

  [[nodiscard]] bool collect_trace() const;
  void set_collect_trace(
    bool enabled
  );

  [[nodiscard]] const equivalence_contract_id& contract() const;
  [[nodiscard]] core::result_t<void> validate() const;

private:
  std::set<rule_id> m_enabled_rules;
  ir::operation_trait m_enabled_traits;
  canonicalization_limits m_limits;
  bool m_collect_trace = false;
};

} // namespace sivra::canonicalizer
