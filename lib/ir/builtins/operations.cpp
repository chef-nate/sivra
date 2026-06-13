#include <sivra/ir/builtins/operations.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace sivra::ir {

core::result_t<builtin_operation_ids> register_builtin_operations(
  operation_catalogue_builder& builder
) {
  const operation_signature unary_same_type{
    .arity =
      {
        .minimum = 1,
        .maximum = 1,
      },
    .operand_types = operand_type_constraint::same_as_result,
  };
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

  const auto zero = operation_constant{well_known_constant::zero};
  const auto one = operation_constant{well_known_constant::one};
  const auto all_bits_set = operation_constant{well_known_constant::all_bits_set};
  const auto negative_infinity = operation_constant{
    f32_constant{.bits = 0xFF80'0000U},
  };
  const auto positive_infinity = operation_constant{
    f32_constant{.bits = 0x7F80'0000U},
  };
  const auto semantics_with_evaluator =
    [](operation_key evaluator_key, operation_semantics semantics = {}) {
      semantics.evaluator_key = std::move(evaluator_key);
      return semantics;
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
          .evaluator_key = operation_key("add"),
          .identity = zero,
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
          .evaluator_key = operation_key("multiply"),
          .identity = one,
          .annihilator = zero,
        },
    },
    operation_registration{
      .key = "subtract",
      .name = "subtract",
      .signature = binary_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .evaluator_key = operation_key("subtract"),
          .right_identity = zero,
        },
    },
    operation_registration{
      .key = "divide",
      .name = "divide",
      .signature = binary_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .evaluator_key = operation_key("divide"),
          .right_identity = one,
        },
    },
    operation_registration{
      .key = "maximum",
      .name = "maximum",
      .signature = variadic_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative |
                    operation_trait::idempotent,
          .evaluator_key = operation_key("maximum"),
          .identity = negative_infinity,
          .annihilator = positive_infinity,
        },
    },
    operation_registration{
      .key = "minimum",
      .name = "minimum",
      .signature = variadic_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative |
                    operation_trait::idempotent,
          .evaluator_key = operation_key("minimum"),
          .identity = positive_infinity,
          .annihilator = negative_infinity,
        },
    },
    operation_registration{
      .key = "sqrt",
      .name = "sqrt",
      .signature = unary_same_type,
      .attribute_schema = {},
      .semantics = semantics_with_evaluator(operation_key("sqrt")),
    },
    operation_registration{
      .key = "reciprocal",
      .name = "reciprocal",
      .signature = unary_same_type,
      .attribute_schema = {},
      .semantics = semantics_with_evaluator(operation_key("reciprocal")),
    },
    operation_registration{
      .key = "reciprocal_sqrt",
      .name = "reciprocal_sqrt",
      .signature = unary_same_type,
      .attribute_schema = {},
      .semantics = semantics_with_evaluator(operation_key("reciprocal_sqrt")),
    },
    operation_registration{
      .key = "square",
      .name = "square",
      .signature = unary_same_type,
      .attribute_schema = {},
      .semantics = semantics_with_evaluator(operation_key("square")),
    },
    operation_registration{
      .key = "bit_and",
      .name = "bit_and",
      .signature = variadic_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative |
                    operation_trait::idempotent,
          .evaluator_key = operation_key("bit_and"),
          .identity = all_bits_set,
          .annihilator = zero,
        },
    },
    operation_registration{
      .key = "bit_and_not",
      .name = "bit_and_not",
      .signature = binary_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .evaluator_key = operation_key("bit_and_not"),
          .left_identity = zero,
          .right_annihilator = zero,
          .notes = "bit_and_not(mask, value) computes (~mask) & value",
        },
    },
    operation_registration{
      .key = "bit_or",
      .name = "bit_or",
      .signature = variadic_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative |
                    operation_trait::idempotent,
          .evaluator_key = operation_key("bit_or"),
          .identity = zero,
          .annihilator = all_bits_set,
        },
    },
    operation_registration{
      .key = "bit_xor",
      .name = "bit_xor",
      .signature = variadic_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .traits = operation_trait::associative | operation_trait::commutative,
          .evaluator_key = operation_key("bit_xor"),
          .identity = zero,
        },
    },
    operation_registration{
      .key = "copy",
      .name = "copy",
      .signature = unary_same_type,
      .attribute_schema = {},
      .semantics =
        operation_semantics{
          .evaluator_key = operation_key("copy"),
          .notes = "Identity copy of one value",
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
    .divide = (*identifiers)[3],
    .maximum = (*identifiers)[4],
    .minimum = (*identifiers)[5],
    .sqrt = (*identifiers)[6],
    .reciprocal = (*identifiers)[7],
    .reciprocal_sqrt = (*identifiers)[8],
    .square = (*identifiers)[9],
    .bit_and = (*identifiers)[10],
    .bit_and_not = (*identifiers)[11],
    .bit_or = (*identifiers)[12],
    .bit_xor = (*identifiers)[13],
    .copy = (*identifiers)[14],
  };
}

} // namespace sivra::ir
