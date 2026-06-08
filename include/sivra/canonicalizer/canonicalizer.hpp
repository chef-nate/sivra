#pragma once

#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/operation_registry.hpp>

#include <cstdint>
#include <span>
#include <vector>

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

/**
 * @struct result
 * @brief Holds the result of canonicalizing one or more root expressions.
 *
 * roots preserve the order of the input root span passed to engine::canonicalize().
 */
struct result {
  ir::expression_graph graph;
  std::vector<ir::node_id> roots;
};

/**
 * @struct single_result
 * @brief Holds the result of canonicalizing one root expression.
 */
struct single_result {
  ir::expression_graph graph;
  ir::node_id root;
};

/**
 * @class engine
 * @brief Applies canonicalization rules to IR expression graphs.
 */
class engine {
public:
  /**
   * @brief Creates an engine using operation definitions from the given registry.
   */
  explicit engine(
    const ir::operation_registry& operations,
    options config = {}
  );

  engine(
    const engine&
  ) = default;

  engine(
    engine&&
  ) = default;

  engine& operator=(
    const engine&
  ) = delete;

  engine& operator=(
    engine&&
  ) = delete;

  /**
   * @brief Returns the operation registry used by this engine.
   */
  [[nodiscard]] const ir::operation_registry& operations() const;

  /**
   * @brief Returns the options used by this engine.
   */
  [[nodiscard]] const options& config() const;

  /**
   * @brief Canonicalizes one or more root expressions.
   *
   * The returned result contains a fresh graph, with roots preserving the order of
   * the input root span.
   */
  [[nodiscard]] result canonicalize(
    const ir::expression_graph& graph,
    std::span<const ir::node_id> roots
  ) const;

  /**
   * @brief Canonicalizes one root expression.
   */
  [[nodiscard]] single_result canonicalize(
    const ir::expression_graph& graph,
    ir::node_id root
  ) const;

private:
  const ir::operation_registry& m_operations;
  options m_options;
};

} // namespace sivra::canonicalizer
