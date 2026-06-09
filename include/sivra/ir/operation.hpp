#pragma once

#include "id.hpp"
#include "scalar_type.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sivra::ir {

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
 * @brief Inverts an operation_trait mask.
 */
constexpr operation_trait operator~(
  operation_trait value
) {
  return static_cast<operation_trait>(~static_cast<std::uint32_t>(value));
}

/**
 * @struct operation_constant
 * @brief Describes an untyped algebraic constant for an operation.
 *
 * operation_constant stores the recovered value for an identity or annihilator.
 * The expression using the operation supplies the concrete result type.
 */
struct operation_constant {
  recovered_float_t value;
};

/**
 * @struct operation_semantics
 * @brief Describes algebraic behavior associated with an operation.
 */
struct operation_semantics {
  operation_trait traits = operation_trait::none;
  std::optional<operation_constant> identity;
  std::optional<operation_constant> annihilator;
  std::string notes;
};

/**
 * @class operation_def
 * @brief Describes an operation kind known to the IR.
 *
 * operation_def stores the operation's stable operation_id, display name, and
 * algebraic semantics.
 */
class operation_def {
public:
  /**
   * @brief Creates an operation definition.
   *
   * @param id operation_id assigned by an operation_registry.
   * @param name Human-readable operation name.
   * @param semantics Algebraic semantics associated with the operation.
   */
  operation_def(
    operation_id id,
    std::string name,
    operation_semantics semantics = {}
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
   * @brief Returns the algebraic semantics associated with this operation.
   */
  const operation_semantics& semantics() const;

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
  operation_semantics m_semantics;
};

} // namespace sivra::ir
