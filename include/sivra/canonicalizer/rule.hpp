#pragma once

#include <cstdint>

namespace sivra::canonicalizer {

/**
 * @enum rule
 * @brief Canonicalization rules independent of operation traits.
 *
 * rule controls transformations that are not represented by ir::operation_trait.
 * Trait-driven behavior is controlled separately through options::enabled_traits.
 */
enum class rule : std::uint32_t {
  none = 0,                       ///< No canonicalization rules.
  identity_elimination = 1u << 0, ///< Removes identity operands.
  annihilator_collapse = 1u << 1, ///< Collapses expressions containing annihilators.
};

/**
 * @brief Combines rule values into a rule mask.
 */
constexpr rule operator|(
  rule lhs,
  rule rhs
) {
  return static_cast<rule>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

/**
 * @brief Intersects rule values.
 */
constexpr rule operator&(
  rule lhs,
  rule rhs
) {
  return static_cast<rule>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

/**
 * @brief Inverts a rule mask.
 */
constexpr rule operator~(
  rule value
) {
  return static_cast<rule>(~static_cast<std::uint32_t>(value));
}

/**
 * @brief Returns the rule mask enabled by default.
 */
[[nodiscard]] constexpr rule default_enabled_rules() {
  return rule::identity_elimination | rule::annihilator_collapse;
}

} // namespace sivra::canonicalizer
