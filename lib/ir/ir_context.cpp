#include <sivra/ir/ir_context.hpp>

namespace sivra::ir {

operation_registry& ir_context::operations() {
  return m_operations;
}

const operation_registry& ir_context::operations() const {
  return m_operations;
}

type_context& ir_context::types() {
  return m_types;
}

const type_context& ir_context::types() const {
  return m_types;
}

} // namespace sivra::ir
