#include <sivra/program/semantic_effect.hpp>

#include <utility>

namespace sivra::program {

core::result_t<void> validate_vector_value(
  const vector_value& value
) {
  if (auto validated = value.type.validate(); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }
  if (value.type.kind() == ir::value_type_kind::scalar) {
    if (value.lanes.size() != 1) {
      return core::fail<void>(
        "program.semantic_effect.invalid_vector_value",
        "scalar semantic value must contain exactly one lane"
      );
    }
    return {};
  }
  if (value.type.kind() == ir::value_type_kind::vector) {
    if (value.lanes.size() != value.type.lane_count()) {
      return core::fail<void>(
        "program.semantic_effect.invalid_vector_value",
        "vector semantic value lane count does not match its value_type"
      );
    }
    return {};
  }
  return core::fail<void>(
    "program.semantic_effect.invalid_vector_value",
    "semantic vector_value must have a scalar or vector value_type"
  );
}

} // namespace sivra::program
