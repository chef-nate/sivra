#pragma once

#include "id.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace sivra {

/**
 * @enum operation_trait
 * @brief Algebraic traits that describe operation behavior.
 */
enum class operation_trait : std::uint32_t {
  none = 0,              ///< No algebraic traits.
  associative = 1u << 0, ///< Nested uses of the operation may be flattened.
  commutative = 1u << 1, ///< Operand order does not affect the expression.
  idempotent = 1u << 2,  ///< Repeated operands may be collapsed.
};

/**
 * @brief Combines operation_trait values into a trait mask.
 */
constexpr operation_trait operator|(
  operation_trait lhs,
  operation_trait rhs
) {
  return static_cast<operation_trait>(
    static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs)
  );
}

/**
 * @brief Intersects operation_trait values.
 */
constexpr operation_trait operator&(
  operation_trait lhs,
  operation_trait rhs
) {
  return static_cast<operation_trait>(
    static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs)
  );
}

/**
 * @class operation_def
 * @brief Describes an operation kind known to the IR.
 *
 * operation_def stores the operation's stable operation_id, display name, and
 * algebraic traits.
 */
class operation_def {
public:
  /**
   * @brief Creates an operation definition.
   *
   * @param id operation_id assigned by an operation_registry.
   * @param name Human-readable operation name.
   * @param traits Algebraic traits associated with the operation.
   */
  operation_def(
    operation_id id,
    std::string name,
    operation_trait traits
  );

  /**
   * @brief Returns the operation_id assigned to this operation.
   */
  operation_id id() const;

  /**
   * @brief Returns the human-readable operation name.
   */
  std::string_view name() const;

  /**
   * @brief Checks whether this operation has all requested traits.
   *
   * Passing a combined trait mask requires every requested trait to be present.
   */
  bool has_trait(
    operation_trait trait
  ) const;

private:
  operation_id m_id;
  std::string m_name;
  operation_trait m_traits;
};

} // namespace sivra
