#include <sivra/ir/builtins/operations.hpp>

#include <sivra/ir/operation.hpp>

#include <array>

namespace sivra::ir {

builtin_operation_ids register_builtin_operations(
  operation_registry& operations
) {
  const std::array registrations{
    operation_registration{.name = "constant"},
    operation_registration{.name = "symbol"},
    operation_registration{.name = "memory_load"},
    operation_registration{
      .name = "add",
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative,
          .identity = operation_constant{well_known_constant::zero},
        },
    },
    operation_registration{
      .name = "multiply",
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative,
          .identity = operation_constant{well_known_constant::one},
          .annihilator = operation_constant{well_known_constant::zero},
        },
    },
  };

  const auto identifiers = operations.register_operations(registrations);

  return builtin_operation_ids{
    .constant = identifiers[0],
    .symbol = identifiers[1],
    .memory_load = identifiers[2],
    .add = identifiers[3],
    .multiply = identifiers[4],
  };
}

} // namespace sivra::ir
