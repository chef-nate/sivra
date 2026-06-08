#pragma once

#include "rule.hpp"

#include <sivra/ir/operation.hpp>

namespace sivra::canonicalizer {

/**
 * @brief Returns the operation traits enabled by default.
 */
[[nodiscard]] constexpr ir::operation_trait default_enabled_traits() {
  return ir::operation_trait::associative | ir::operation_trait::commutative |
         ir::operation_trait::idempotent;
}

/**
 * @struct options
 * @brief Configures which canonicalization behavior is enabled.
 *
 * enabled_traits controls behavior driven by ir::operation_trait metadata.
 * enabled_rules controls canonicalizer rules independent of operation traits.
 */
struct options {
  ir::operation_trait enabled_traits = default_enabled_traits();
  rule enabled_rules = default_enabled_rules();

  /**
   * @brief Enables canonicalization behavior for the requested operation trait mask.
   */
  void enable_trait(
    ir::operation_trait trait
  );

  /**
   * @brief Disables canonicalization behavior for the requested operation trait mask.
   */
  void disable_trait(
    ir::operation_trait trait
  );

  /**
   * @brief Returns true if every requested operation trait is enabled.
   */
  [[nodiscard]] bool is_trait_enabled(
    ir::operation_trait trait
  ) const;

  /**
   * @brief Enables the requested canonicalization rule mask.
   */
  void enable_rule(
    rule canonicalization_rule
  );

  /**
   * @brief Disables the requested canonicalization rule mask.
   */
  void disable_rule(
    rule canonicalization_rule
  );

  /**
   * @brief Returns true if every requested canonicalization rule is enabled.
   */
  [[nodiscard]] bool is_rule_enabled(
    rule canonicalization_rule
  ) const;
};

} // namespace sivra::canonicalizer
