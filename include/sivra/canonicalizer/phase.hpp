#pragma once

#include "id.hpp"

#include <sivra/core/result.hpp>

#include <span>
#include <string_view>
#include <vector>

namespace sivra::canonicalizer {

class rule_catalogue;

enum class pass_phase {
  validation,
  local_simplification,
  shape_normalization,
  algebraic_collection,
  domain_normalization,
  cleanup,
  verification,
};

struct pass_descriptor {
  pass_phase phase;
  std::string_view name;
};

enum class fixed_point_policy {
  once,
  until_stable,
};

struct pass_plan {
  pass_phase phase;
  fixed_point_policy policy = fixed_point_policy::until_stable;
  std::vector<rule_id> rules;
};

class pass_scheduler {
public:
  explicit pass_scheduler(
    const rule_catalogue& rules
  );

  [[nodiscard]] std::span<const pass_plan> plans() const;
  [[nodiscard]] const pass_plan* plan_for(
    pass_phase phase
  ) const;
  [[nodiscard]] core::result_t<void> validate(
    const rule_catalogue& rules
  ) const;

private:
  std::vector<pass_plan> m_plans;
};

[[nodiscard]] std::span<const pass_descriptor> available_phases();
[[nodiscard]] bool is_known_phase(
  pass_phase phase
);

} // namespace sivra::canonicalizer
