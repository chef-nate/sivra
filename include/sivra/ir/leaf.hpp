#pragma once

#include "constant.hpp"
#include "scalar_type.hpp"

#include <cstddef>
#include <string>
#include <variant>

namespace sivra::ir {

/**
 * @struct memory_ref
 * @brief Describes an abstract memory operand used as a leaf expression.
 *
 * memory_ref identifies memory by scalar type, base register name, and byte
 * offset. It does not refer to host process memory.
 */
struct memory_ref {
  scalar_type scalar;
  std::string base_register;
  std::ptrdiff_t offset;
};

/**
 * @struct symbol_ref
 * @brief Describes a named symbolic value used as a leaf expression.
 *
 * Symbols can represent recovered variables, temporaries, or values whose
 * concrete contents are not known.
 */
struct symbol_ref {
  std::string name;
};

/**
 * @brief Leaf expression value stored by expression_node.
 */
using leaf_type_t = std::variant<memory_ref, constant_value, symbol_ref>;

} // namespace sivra::ir
