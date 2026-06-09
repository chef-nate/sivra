#pragma once

#include "operation_registry.hpp"
#include "type.hpp"

namespace sivra::ir {

/**
 * @class ir_context
 * @brief Owns the operation and type definitions used by IR graphs.
 *
 * An ir_context must outlive every expression_graph that references it,
 * including graphs returned by canonicalization.
 */
class ir_context {
public:
  ir_context() = default;

  ir_context(
    const ir_context&
  ) = delete;

  ir_context(
    ir_context&&
  ) = delete;

  ir_context& operator=(
    const ir_context&
  ) = delete;

  ir_context& operator=(
    ir_context&&
  ) = delete;

  operation_registry& operations();
  const operation_registry& operations() const;

  type_context& types();
  const type_context& types() const;

private:
  operation_registry m_operations;
  type_context m_types;
};

} // namespace sivra::ir
