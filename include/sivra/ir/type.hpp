#pragma once

#include "scalar_type.hpp"

namespace sivra {

/**
 * @class type
 * @brief Describes the result type of an expression.
 *
 * type is intentionally small while the IR only needs scalar lane types.
 */
class type {
public:
  /**
   * @brief Creates a type from a scalar_type.
   */
  explicit type(
    scalar_type scalar
  );

  /**
   * @brief Returns the scalar lane type.
   */
  scalar_type scalar() const;

  bool operator==(
    const type&
  ) const;

private:
  scalar_type m_scalar;
};

} // namespace sivra
