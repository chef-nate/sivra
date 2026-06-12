#pragma once

#include <sivra/canonicalizer/rewrite.hpp>

namespace sivra::canonicalizer {

rewrite_result apply_associative_flattening(
  rewrite_context& context,
  const rewrite_subject& subject
);

rewrite_result apply_commutative_ordering(
  rewrite_context& context,
  const rewrite_subject& subject
);

rewrite_result apply_idempotent_deduplication(
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

rewrite_result apply_same_operand_simplification(
  rewrite_context& context,
  const rewrite_subject& subject
);

rewrite_result apply_constant_folding(
  rewrite_context& context,
  const rewrite_subject& subject
);

rewrite_result apply_coefficient_collection(
  rewrite_context& context,
  const rewrite_subject& subject
);

} // namespace sivra::canonicalizer
