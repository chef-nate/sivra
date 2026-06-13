#include <sivra/canonicalizer/evaluator.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

namespace {

enum class builtin_evaluation {
  add,
  multiply,
  subtract,
  divide,
  maximum,
  minimum,
  sqrt,
  reciprocal,
  reciprocal_sqrt,
  square,
  bit_and,
  bit_and_not,
  bit_or,
  bit_xor,
  copy,
};

bool is_commutative(
  builtin_evaluation evaluation
) {
  switch (evaluation) {
  case builtin_evaluation::add:
  case builtin_evaluation::multiply:
  case builtin_evaluation::maximum:
  case builtin_evaluation::minimum:
  case builtin_evaluation::bit_and:
  case builtin_evaluation::bit_or:
  case builtin_evaluation::bit_xor:
    return true;
  case builtin_evaluation::subtract:
  case builtin_evaluation::divide:
  case builtin_evaluation::sqrt:
  case builtin_evaluation::reciprocal:
  case builtin_evaluation::reciprocal_sqrt:
  case builtin_evaluation::square:
  case builtin_evaluation::bit_and_not:
  case builtin_evaluation::copy:
    return false;
  }
  return false;
}

bool accepts_operand_count(
  builtin_evaluation evaluation,
  std::size_t count
) {
  switch (evaluation) {
  case builtin_evaluation::sqrt:
  case builtin_evaluation::reciprocal:
  case builtin_evaluation::reciprocal_sqrt:
  case builtin_evaluation::square:
  case builtin_evaluation::copy:
    return count == 1;
  case builtin_evaluation::subtract:
  case builtin_evaluation::divide:
  case builtin_evaluation::bit_and_not:
    return count == 2;
  case builtin_evaluation::add:
  case builtin_evaluation::multiply:
  case builtin_evaluation::maximum:
  case builtin_evaluation::minimum:
  case builtin_evaluation::bit_and:
  case builtin_evaluation::bit_or:
  case builtin_evaluation::bit_xor:
    return count >= 2;
  }
  return false;
}

bool supports_result_type(
  builtin_evaluation evaluation,
  const sivra::ir::value_type& result_type
) {
  if (result_type.element_bit_width() != 32) {
    return false;
  }
  const auto category = result_type.category();
  switch (evaluation) {
  case builtin_evaluation::divide:
  case builtin_evaluation::sqrt:
  case builtin_evaluation::reciprocal:
  case builtin_evaluation::reciprocal_sqrt:
    return category == sivra::ir::scalar_category::floating_point;
  case builtin_evaluation::add:
  case builtin_evaluation::multiply:
  case builtin_evaluation::subtract:
  case builtin_evaluation::maximum:
  case builtin_evaluation::minimum:
  case builtin_evaluation::square:
  case builtin_evaluation::bit_and:
  case builtin_evaluation::bit_and_not:
  case builtin_evaluation::bit_or:
  case builtin_evaluation::bit_xor:
  case builtin_evaluation::copy:
    return category == sivra::ir::scalar_category::floating_point ||
           category == sivra::ir::scalar_category::signed_integer;
  }
  return false;
}

std::uint32_t canonical_f32_bits(
  sivra::ir::f32_constant value
) {
  constexpr std::uint32_t sign_mask = 0x8000'0000U;
  constexpr std::uint32_t exponent_mask = 0x7F80'0000U;
  constexpr std::uint32_t fraction_mask = 0x007F'FFFFU;
  constexpr std::uint32_t quiet_nan = 0x7FC0'0000U;

  if ((value.bits & ~sign_mask) == 0) {
    return 0;
  }
  if ((value.bits & exponent_mask) == exponent_mask && (value.bits & fraction_mask) != 0) {
    return quiet_nan;
  }
  return value.bits;
}

sivra::ir::f32_constant canonical_f32(
  float value
) {
  auto constant = sivra::ir::f32_constant::from_value(value);
  constant.bits = canonical_f32_bits(constant);
  return constant;
}

std::uint32_t sortable_bits(
  const sivra::ir::scalar_constant_t& value
) {
  if (const auto* floating = std::get_if<sivra::ir::f32_constant>(&value)) {
    return canonical_f32_bits(*floating);
  }
  return std::get<sivra::ir::i32_constant>(value).bits;
}

bool constant_less(
  const sivra::ir::constant_value* lhs,
  const sivra::ir::constant_value* rhs
) {
  for (std::size_t index = 0; index < lhs->element_count(); ++index) {
    const auto lhs_bits = sortable_bits(lhs->element(index));
    const auto rhs_bits = sortable_bits(rhs->element(index));
    if (lhs_bits != rhs_bits) {
      return lhs_bits < rhs_bits;
    }
  }
  return lhs->is_splat() < rhs->is_splat();
}

sivra::ir::scalar_constant_t evaluate_scalar(
  builtin_evaluation evaluation,
  std::span<const sivra::ir::constant_value* const> operands,
  std::size_t lane,
  const sivra::ir::value_type& result_type
) {
  if (result_type.category() == sivra::ir::scalar_category::floating_point) {
    const auto first = std::get<sivra::ir::f32_constant>(operands.front()->element(lane));
    if (evaluation == builtin_evaluation::copy) {
      return first;
    }
    if (evaluation == builtin_evaluation::bit_and ||
        evaluation == builtin_evaluation::bit_and_not || evaluation == builtin_evaluation::bit_or ||
        evaluation == builtin_evaluation::bit_xor) {
      auto bits = first.bits;
      for (const auto& operand : operands.subspan(1)) {
        const auto rhs = std::get<sivra::ir::f32_constant>(operand->element(lane)).bits;
        switch (evaluation) {
        case builtin_evaluation::bit_and:
          bits &= rhs;
          break;
        case builtin_evaluation::bit_and_not:
          bits = ~bits & rhs;
          break;
        case builtin_evaluation::bit_or:
          bits |= rhs;
          break;
        case builtin_evaluation::bit_xor:
          bits ^= rhs;
          break;
        default:
          break;
        }
      }
      return sivra::ir::f32_constant{.bits = bits};
    }

    auto value = first.value();
    if (evaluation == builtin_evaluation::sqrt) {
      return canonical_f32(std::sqrt(value));
    }
    if (evaluation == builtin_evaluation::reciprocal) {
      return canonical_f32(1.0F / value);
    }
    if (evaluation == builtin_evaluation::reciprocal_sqrt) {
      return canonical_f32(1.0F / std::sqrt(value));
    }
    if (evaluation == builtin_evaluation::square) {
      return canonical_f32(value * value);
    }
    for (const auto& operand : operands.subspan(1)) {
      const auto rhs = std::get<sivra::ir::f32_constant>(operand->element(lane)).value();
      switch (evaluation) {
      case builtin_evaluation::add:
        value += rhs;
        break;
      case builtin_evaluation::multiply:
        value *= rhs;
        break;
      case builtin_evaluation::subtract:
        value -= rhs;
        break;
      case builtin_evaluation::divide:
        value /= rhs;
        break;
      case builtin_evaluation::maximum:
        value = std::fmax(value, rhs);
        break;
      case builtin_evaluation::minimum:
        value = std::fmin(value, rhs);
        break;
      default:
        break;
      }
    }
    return canonical_f32(value);
  }

  auto bits = std::get<sivra::ir::i32_constant>(operands.front()->element(lane)).bits;
  if (evaluation == builtin_evaluation::copy) {
    return sivra::ir::i32_constant{.bits = bits};
  }
  if (evaluation == builtin_evaluation::square) {
    bits *= bits;
    return sivra::ir::i32_constant{.bits = bits};
  }
  for (const auto& operand : operands.subspan(1)) {
    const auto rhs = std::get<sivra::ir::i32_constant>(operand->element(lane)).bits;
    switch (evaluation) {
    case builtin_evaluation::add:
      bits += rhs;
      break;
    case builtin_evaluation::multiply:
      bits *= rhs;
      break;
    case builtin_evaluation::subtract:
      bits -= rhs;
      break;
    case builtin_evaluation::bit_and:
      bits &= rhs;
      break;
    case builtin_evaluation::bit_and_not:
      bits = ~bits & rhs;
      break;
    case builtin_evaluation::bit_or:
      bits |= rhs;
      break;
    case builtin_evaluation::bit_xor:
      bits ^= rhs;
      break;
    case builtin_evaluation::maximum:
      if (std::bit_cast<std::int32_t>(rhs) > std::bit_cast<std::int32_t>(bits)) {
        bits = rhs;
      }
      break;
    case builtin_evaluation::minimum:
      if (std::bit_cast<std::int32_t>(rhs) < std::bit_cast<std::int32_t>(bits)) {
        bits = rhs;
      }
      break;
    default:
      break;
    }
  }
  return sivra::ir::i32_constant{.bits = bits};
}

sivra::canonicalizer::evaluation_result evaluate_builtin(
  builtin_evaluation evaluation,
  std::span<const sivra::ir::constant_value> operands,
  const sivra::ir::operation_attributes& attributes,
  sivra::ir::value_type result_type
) {
  if (!accepts_operand_count(evaluation, operands.size()) || !attributes.empty() ||
      !supports_result_type(evaluation, result_type)) {
    return sivra::canonicalizer::no_evaluation{};
  }
  for (const auto& operand : operands) {
    if (operand.result_type() != result_type) {
      return sivra::canonicalizer::no_evaluation{};
    }
  }

  std::vector<const sivra::ir::constant_value*> ordered_operands;
  ordered_operands.reserve(operands.size());
  for (const auto& operand : operands) {
    ordered_operands.push_back(&operand);
  }
  if (is_commutative(evaluation)) {
    std::ranges::stable_sort(ordered_operands, constant_less);
  }

  sivra::core::result_t<sivra::ir::constant_value> value =
    result_type.kind() == sivra::ir::value_type_kind::scalar
      ? sivra::ir::constant_value::scalar(
          result_type, evaluate_scalar(evaluation, ordered_operands, 0, result_type)
        )
      : [&] {
          std::vector<sivra::ir::scalar_constant_t> elements;
          elements.reserve(result_type.lane_count());
          for (std::size_t lane = 0; lane < result_type.lane_count(); ++lane) {
            elements.push_back(evaluate_scalar(evaluation, ordered_operands, lane, result_type));
          }
          return sivra::ir::constant_value::aggregate(result_type, std::move(elements));
        }();
  if (!value.has_value()) {
    return sivra::canonicalizer::invalid_evaluation{
      .diagnostic = std::move(value.error().front()),
    };
  }
  return sivra::canonicalizer::evaluated_constant{.value = std::move(*value)};
}

#define SIVRA_DEFINE_EVALUATOR(name)                                                               \
  sivra::canonicalizer::evaluation_result evaluate_##name(                                         \
    std::span<const sivra::ir::constant_value> operands,                                           \
    const sivra::ir::operation_attributes& attributes,                                             \
    sivra::ir::value_type result_type                                                              \
  ) {                                                                                              \
    return evaluate_builtin(                                                                       \
      builtin_evaluation::name, operands, attributes, std::move(result_type)                       \
    );                                                                                             \
  }

SIVRA_DEFINE_EVALUATOR(
  add
)
SIVRA_DEFINE_EVALUATOR(
  multiply
)
SIVRA_DEFINE_EVALUATOR(
  subtract
)
SIVRA_DEFINE_EVALUATOR(
  divide
)
SIVRA_DEFINE_EVALUATOR(
  maximum
)
SIVRA_DEFINE_EVALUATOR(
  minimum
)
SIVRA_DEFINE_EVALUATOR(
  sqrt
)
SIVRA_DEFINE_EVALUATOR(
  reciprocal
)
SIVRA_DEFINE_EVALUATOR(
  reciprocal_sqrt
)
SIVRA_DEFINE_EVALUATOR(
  square
)
SIVRA_DEFINE_EVALUATOR(
  bit_and
)
SIVRA_DEFINE_EVALUATOR(
  bit_and_not
)
SIVRA_DEFINE_EVALUATOR(
  bit_or
)
SIVRA_DEFINE_EVALUATOR(
  bit_xor
)
SIVRA_DEFINE_EVALUATOR(
  copy
)

#undef SIVRA_DEFINE_EVALUATOR

} // namespace

namespace sivra::canonicalizer {

evaluator_catalogue::evaluator_catalogue(
  std::vector<operation_evaluator> evaluators
)
    : m_evaluators(std::move(evaluators)) {
}

core::result_t<std::shared_ptr<const evaluator_catalogue>> evaluator_catalogue::create(
  std::vector<operation_evaluator> evaluators
) {
  std::set<ir::operation_key> operations;
  for (const auto& evaluator : evaluators) {
    if (evaluator.operation.empty() || evaluator.operation.version() == 0 ||
        evaluator.evaluate == nullptr) {
      return core::fail<std::shared_ptr<const evaluator_catalogue>>(
        "canonicalizer.evaluator_catalogue.invalid_evaluator",
        "operation evaluators require a stable operation key and evaluate function"
      );
    }
    if (!operations.insert(evaluator.operation).second) {
      return core::fail<std::shared_ptr<const evaluator_catalogue>>(
        "canonicalizer.evaluator_catalogue.duplicate_evaluator",
        "operation evaluator is registered more than once"
      );
    }
  }
  return std::shared_ptr<const evaluator_catalogue>(new evaluator_catalogue(std::move(evaluators)));
}

const operation_evaluator* evaluator_catalogue::find(
  const ir::operation_key& operation
) const {
  const auto found =
    std::ranges::find(m_evaluators, operation, [](const operation_evaluator& evaluator) {
      return evaluator.operation;
    });
  return found == m_evaluators.end() ? nullptr : &*found;
}

std::span<const operation_evaluator> evaluator_catalogue::evaluators() const {
  return m_evaluators;
}

std::shared_ptr<const evaluator_catalogue> builtin_evaluator_catalogue() {
  static const auto catalogue = [] {
    auto created = evaluator_catalogue::create(
      {
        {.operation = "add", .evaluate = evaluate_add},
        {.operation = "multiply", .evaluate = evaluate_multiply},
        {.operation = "subtract", .evaluate = evaluate_subtract},
        {.operation = "divide", .evaluate = evaluate_divide},
        {.operation = "maximum", .evaluate = evaluate_maximum},
        {.operation = "minimum", .evaluate = evaluate_minimum},
        {.operation = "sqrt", .evaluate = evaluate_sqrt},
        {.operation = "reciprocal", .evaluate = evaluate_reciprocal},
        {.operation = "reciprocal_sqrt", .evaluate = evaluate_reciprocal_sqrt},
        {.operation = "square", .evaluate = evaluate_square},
        {.operation = "bit_and", .evaluate = evaluate_bit_and},
        {.operation = "bit_and_not", .evaluate = evaluate_bit_and_not},
        {.operation = "bit_or", .evaluate = evaluate_bit_or},
        {.operation = "bit_xor", .evaluate = evaluate_bit_xor},
        {.operation = "copy", .evaluate = evaluate_copy},
      }
    );
    if (!created.has_value()) {
      std::terminate();
    }
    return *created;
  }();
  return catalogue;
}

} // namespace sivra::canonicalizer
