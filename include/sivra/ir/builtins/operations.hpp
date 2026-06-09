#pragma once

#include <sivra/ir/id.hpp>
#include <sivra/ir/operation_registry.hpp>

namespace sivra::ir {

/**
 * @struct builtin_operation_ids
 * @brief operation_id values assigned to the standard IR operation set.
 */
struct builtin_operation_ids {
  operation_id constant;
  operation_id symbol;
  operation_id memory_load;
  operation_id add;
  operation_id multiply;
};

/**
 * @brief Registers the standard IR operation set.
 *
 * This function registers architecture-neutral operation definitions used by
 * graph construction and canonicalization. Calling it more than once for the
 * same operation_registry throws through operation_registry duplicate-name
 * checks.
 */
[[nodiscard]] builtin_operation_ids register_builtin_operations(
  operation_registry& operations
);

} // namespace sivra::ir
