#include "rules.hpp"

#include <array>

namespace sivra::canonicalizer {

std::span<const rule_descriptor> scheduled_rules() {
  static const std::array scheduled{
    rule_descriptor{
      .id = builtin_rules::associative_flattening,
      .phase = pass_phase::local_simplification,
      .apply = apply_associative_flattening,
    },
    rule_descriptor{
      .id = builtin_rules::identity_elimination,
      .phase = pass_phase::local_simplification,
      .apply = apply_identity_elimination,
    },
    rule_descriptor{
      .id = builtin_rules::annihilator_collapse,
      .phase = pass_phase::local_simplification,
      .apply = apply_annihilator_collapse,
    },
  };
  return scheduled;
}

} // namespace sivra::canonicalizer
