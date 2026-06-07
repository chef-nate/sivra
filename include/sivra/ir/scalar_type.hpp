#pragma once

namespace sivra {

/**
 * @brief Host-side floating-point type used to store recovered scalar constants.
 */
using recovered_float = double;

/**
 * @enum scalar_type
 * @brief Scalar lane types represented by the IR.
 */
enum class scalar_type {
  unknown, ///< Type is not known or has not been assigned.
  f32,     ///< 32-bit floating-point scalar.
  i32,     ///< 32-bit integer scalar.
};

} // namespace sivra
