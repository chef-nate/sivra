#include <sivra/canonicalizer/rewrite.hpp>

#include "rules/rules.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <set>
#include <utility>

namespace sivra::canonicalizer {

rule_catalogue::rule_catalogue(
  std::vector<rewrite_rule> rules
)
    : m_rules(std::move(rules)) {
}

core::result_t<std::shared_ptr<const rule_catalogue>> rule_catalogue::create(
  std::vector<rewrite_rule> rules
) {
  std::set<rule_id> identifiers;
  for (const auto& rule : rules) {
    if (rule.metadata.id.key().empty() || rule.metadata.name.empty() || rule.apply == nullptr) {
      return core::fail<std::shared_ptr<const rule_catalogue>>(
        "canonicalizer.rule_catalogue.invalid_rule",
        "rewrite rules require an identifier, name, and apply function"
      );
    }
    if (!is_known_phase(rule.metadata.phase)) {
      return core::fail<std::shared_ptr<const rule_catalogue>>(
        "canonicalizer.rule_catalogue.invalid_phase",
        "rewrite rule references an unknown canonicalization phase"
      );
    }
    if (!identifiers.insert(rule.metadata.id).second) {
      return core::fail<std::shared_ptr<const rule_catalogue>>(
        "canonicalizer.rule_catalogue.duplicate_rule",
        "rewrite rule identifier is registered more than once"
      );
    }
  }

  std::ranges::sort(rules, [](const rewrite_rule& lhs, const rewrite_rule& rhs) {
    if (lhs.metadata.phase != rhs.metadata.phase) {
      return lhs.metadata.phase < rhs.metadata.phase;
    }
    if (lhs.metadata.priority != rhs.metadata.priority) {
      return lhs.metadata.priority < rhs.metadata.priority;
    }
    return lhs.metadata.id < rhs.metadata.id;
  });
  return std::shared_ptr<const rule_catalogue>(new rule_catalogue(std::move(rules)));
}

const rewrite_rule* rule_catalogue::find(
  const rule_id& id
) const {
  const auto found =
    std::ranges::find(m_rules, id, [](const rewrite_rule& rule) { return rule.metadata.id; });
  return found == m_rules.end() ? nullptr : &*found;
}

std::span<const rewrite_rule> rule_catalogue::rules() const {
  return m_rules;
}

std::shared_ptr<const rule_catalogue> builtin_rule_catalogue() {
  static const auto catalogue = [] {
    std::vector<rewrite_rule> rules{
      {
        .metadata =
          {
            .id = builtin_rules::copy_elimination,
            .name = "copy_elimination",
            .phase = pass_phase::local_simplification,
            .priority = 50,
            .description = "Remove identity copy operations.",
            .decreasing_measure = "operation count",
          },
        .apply = apply_copy_elimination,
      },
      {
        .metadata =
          {
            .id = builtin_rules::identity_elimination,
            .name = "identity_elimination",
            .phase = pass_phase::local_simplification,
            .priority = 100,
            .description = "Remove operation identity operands.",
            .decreasing_measure = "operand count or operation depth",
          },
        .apply = apply_identity_elimination,
      },
      {
        .metadata =
          {
            .id = builtin_rules::annihilator_collapse,
            .name = "annihilator_collapse",
            .phase = pass_phase::local_simplification,
            .priority = 200,
            .description = "Replace an operation containing its annihilator.",
            .decreasing_measure = "operation count",
          },
        .apply = apply_annihilator_collapse,
      },
      {
        .metadata =
          {
            .id = builtin_rules::bitwise_simplification,
            .name = "bitwise_simplification",
            .phase = pass_phase::local_simplification,
            .priority = 225,
            .description = "Apply directional and cancellation bitwise identities.",
            .decreasing_measure = "operand or operation count",
          },
        .apply = apply_bitwise_simplification,
      },
      {
        .metadata =
          {
            .id = builtin_rules::same_operand_simplification,
            .name = "same_operand_simplification",
            .phase = pass_phase::local_simplification,
            .priority = 250,
            .description = "Simplify operations with structurally equal operands.",
            .decreasing_measure = "operation count",
          },
        .apply = apply_same_operand_simplification,
      },
      {
        .metadata =
          {
            .id = builtin_rules::constant_folding,
            .name = "constant_folding",
            .phase = pass_phase::local_simplification,
            .priority = 300,
            .description = "Evaluate operations with constant operands.",
            .decreasing_measure = "operation count",
          },
        .apply = apply_constant_folding,
      },
      {
        .metadata =
          {
            .id = builtin_rules::mixed_constant_aggregation,
            .name = "mixed_constant_aggregation",
            .phase = pass_phase::local_simplification,
            .priority = 350,
            .description = "Aggregate constant operands within associative expressions.",
            .decreasing_measure = "constant operand count",
          },
        .apply = apply_mixed_constant_aggregation,
      },
      {
        .metadata =
          {
            .id = builtin_rules::subtraction_normalization,
            .name = "subtraction_normalization",
            .phase = pass_phase::shape_normalization,
            .priority = 50,
            .description = "Normalize subtraction to addition with a negative coefficient.",
            .decreasing_measure = "subtraction operation count",
            .may_grow = true,
          },
        .apply = apply_subtraction_normalization,
      },
      {
        .metadata =
          {
            .id = builtin_rules::associative_flattening,
            .name = "associative_flattening",
            .phase = pass_phase::shape_normalization,
            .priority = 100,
            .description = "Flatten nested associative operations.",
            .decreasing_measure = "nested associative depth",
            .may_grow = true,
          },
        .apply = apply_associative_flattening,
      },
      {
        .metadata =
          {
            .id = builtin_rules::commutative_ordering,
            .name = "commutative_ordering",
            .phase = pass_phase::shape_normalization,
            .priority = 200,
            .description = "Order commutative operands structurally.",
            .decreasing_measure = "operand inversion count",
          },
        .apply = apply_commutative_ordering,
      },
      {
        .metadata =
          {
            .id = builtin_rules::idempotent_deduplication,
            .name = "idempotent_deduplication",
            .phase = pass_phase::shape_normalization,
            .priority = 300,
            .description = "Remove duplicate idempotent operands.",
            .decreasing_measure = "operand count",
          },
        .apply = apply_idempotent_deduplication,
      },
      {
        .metadata =
          {
            .id = builtin_rules::coefficient_collection,
            .name = "coefficient_collection",
            .phase = pass_phase::algebraic_collection,
            .priority = 100,
            .description = "Collect coefficients of equal additive terms.",
            .decreasing_measure = "number of repeated additive bases",
            .may_grow = true,
          },
        .apply = apply_coefficient_collection,
      },
      {
        .metadata =
          {
            .id = builtin_rules::division_reciprocal_simplification,
            .name = "division_reciprocal_simplification",
            .phase = pass_phase::algebraic_collection,
            .priority = 200,
            .description = "Simplify division and reciprocal expressions.",
            .decreasing_measure = "division depth or inverse pair count",
            .may_grow = true,
          },
        .apply = apply_division_reciprocal_simplification,
      },
      {
        .metadata =
          {
            .id = builtin_rules::square_simplification,
            .name = "square_simplification",
            .phase = pass_phase::algebraic_collection,
            .priority = 300,
            .description = "Normalize repeated factors and square-root pairs.",
            .decreasing_measure = "repeated factor count or inverse pair depth",
          },
        .apply = apply_square_simplification,
      },
    };
    auto created = rule_catalogue::create(std::move(rules));
    if (!created.has_value()) {
      std::terminate();
    }
    return *created;
  }();
  return catalogue;
}

} // namespace sivra::canonicalizer
