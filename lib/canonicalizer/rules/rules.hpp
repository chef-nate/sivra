#pragma once

#include "../rewrite.hpp"

#include <sivra/canonicalizer/rule.hpp>

#include <span>

namespace sivra::canonicalizer {

using apply_rule_fn = rewrite_result (*)(
  rewrite_context& context,
  const rewrite_subject& subject
);

struct rule_descriptor {
  rule_id id;
  pass_phase phase;
  apply_rule_fn apply;
};

rewrite_result apply_associative_flattening(
  rewrite_context& context,
  const rewrite_subject& subject
);

rewrite_result apply_identity_elimination(
  rewrite_context& context,
  const rewrite_subject& subject
);

rewrite_result apply_annihilator_collapse(
  rewrite_context& context,
  const rewrite_subject& subject
);

std::span<const rule_descriptor> scheduled_rules();

} // namespace sivra::canonicalizer
