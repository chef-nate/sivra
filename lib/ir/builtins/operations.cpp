#include <sivra/ir/builtins/operations.hpp>

#include <array>
#include <utility>

namespace sivra::ir {

core::result_t<builtin_operation_ids> register_builtin_operations(
  operation_catalogue_builder& builder
) {
  const operation_signature variadic_same_type{
    .arity =
      {
        .minimum = 2,
        .maximum = std::nullopt,
      },
    .operand_types = operand_type_constraint::same_as_result,
  };
  const operation_signature binary_same_type{
    .arity =
      {
        .minimum = 2,
        .maximum = 2,
      },
    .operand_types = operand_type_constraint::same_as_result,
  };

  const std::array registrations{
    operation_registration{
      .key = "add",
      .name = "add",
      .signature = variadic_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative,
          .identity = operation_constant{well_known_constant::zero},
        },
    },
    operation_registration{
      .key = "multiply",
      .name = "multiply",
      .signature = variadic_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative,
          .identity = operation_constant{well_known_constant::one},
          .annihilator = operation_constant{well_known_constant::zero},
        },
    },
    operation_registration{
      .key = "subtract",
      .name = "subtract",
      .signature = binary_same_type,
      .attribute_schema = {},
    },
    operation_registration{
      .key = "maximum",
      .name = "maximum",
      .signature = binary_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::commutative | operation_trait::idempotent,
        },
    },
  };

  auto identifiers = builder.register_operations(registrations);
  if (!identifiers.has_value()) {
    return std::unexpected(std::move(identifiers.error()));
  }

  return builtin_operation_ids{
    .add = (*identifiers)[0],
    .multiply = (*identifiers)[1],
    .subtract = (*identifiers)[2],
    .maximum = (*identifiers)[3],
  };
}

} // namespace sivra::ir
