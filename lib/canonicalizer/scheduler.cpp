#include <sivra/canonicalizer/phase.hpp>

#include <sivra/canonicalizer/rewrite.hpp>

#include <algorithm>
#include <array>

namespace {

constexpr std::array phase_descriptors{
  sivra::canonicalizer::pass_descriptor{
    .phase = sivra::canonicalizer::pass_phase::validation,
    .name = "validation",
  },
  sivra::canonicalizer::pass_descriptor{
    .phase = sivra::canonicalizer::pass_phase::local_simplification,
    .name = "local_simplification",
  },
  sivra::canonicalizer::pass_descriptor{
    .phase = sivra::canonicalizer::pass_phase::shape_normalization,
    .name = "shape_normalization",
  },
  sivra::canonicalizer::pass_descriptor{
    .phase = sivra::canonicalizer::pass_phase::algebraic_collection,
    .name = "algebraic_collection",
  },
  sivra::canonicalizer::pass_descriptor{
    .phase = sivra::canonicalizer::pass_phase::domain_normalization,
    .name = "domain_normalization",
  },
  sivra::canonicalizer::pass_descriptor{
    .phase = sivra::canonicalizer::pass_phase::cleanup,
    .name = "cleanup",
  },
  sivra::canonicalizer::pass_descriptor{
    .phase = sivra::canonicalizer::pass_phase::verification,
    .name = "verification",
  },
};

} // namespace

namespace sivra::canonicalizer {

pass_scheduler::pass_scheduler(
  const rule_catalogue& rules
) {
  m_plans.reserve(phase_descriptors.size());
  for (const auto& descriptor : phase_descriptors) {
    pass_plan plan{
      .phase = descriptor.phase,
      .policy =
        descriptor.phase == pass_phase::validation || descriptor.phase == pass_phase::verification
          ? fixed_point_policy::once
          : fixed_point_policy::until_stable,
    };
    for (const auto& rule : rules.rules()) {
      if (rule.metadata.phase == descriptor.phase) {
        plan.rules.push_back(rule.metadata.id);
      }
    }
    m_plans.push_back(std::move(plan));
  }
}

std::span<const pass_plan> pass_scheduler::plans() const {
  return m_plans;
}

const pass_plan* pass_scheduler::plan_for(
  pass_phase phase
) const {
  const auto found = std::ranges::find(m_plans, phase, &pass_plan::phase);
  return found == m_plans.end() ? nullptr : &*found;
}

core::result_t<void> pass_scheduler::validate(
  const rule_catalogue& rules
) const {
  for (const auto& plan : m_plans) {
    for (const auto& identifier : plan.rules) {
      const auto* rule = rules.find(identifier);
      if (rule == nullptr || rule->metadata.phase != plan.phase) {
        return core::fail<void>(
          "canonicalizer.scheduler.invalid_plan",
          "pass plan references a missing rule or a rule from another phase"
        );
      }
    }
  }
  return {};
}

std::span<const pass_descriptor> available_phases() {
  return phase_descriptors;
}

bool is_known_phase(
  pass_phase phase
) {
  return std::ranges::any_of(phase_descriptors, [phase](const pass_descriptor& descriptor) {
    return descriptor.phase == phase;
  });
}

} // namespace sivra::canonicalizer
