#include <sivra/ir/builtins/operations.hpp>

#include <sivra/ir/operation.hpp>

namespace sivra::ir {

builtin_operation_ids register_builtin_operations(
  operation_registry& operations
) {
  const auto constant = operations.register_operation("constant");
  const auto symbol = operations.register_operation("symbol");
  const auto memory_load = operations.register_operation("memory_load");

  const auto add = operations.register_operation(
    "add",
    operation_semantics{
      .traits = operation_trait::associative | operation_trait::commutative,
      .identity = operation_constant{well_known_constant::zero},
    }
  );

  const auto multiply = operations.register_operation(
    "multiply",
    operation_semantics{
      .traits = operation_trait::associative | operation_trait::commutative,
      .identity = operation_constant{well_known_constant::one},
      .annihilator = operation_constant{well_known_constant::zero},
    }
  );

  return builtin_operation_ids{
    .constant = constant,
    .symbol = symbol,
    .memory_load = memory_load,
    .add = add,
    .multiply = multiply,
  };
}

} // namespace sivra::ir
