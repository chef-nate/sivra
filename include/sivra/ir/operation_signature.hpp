#pragma once

#include "operation_attribute.hpp"
#include "value_type.hpp"

#include <sivra/core/result.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace sivra::ir {

struct arity_constraint {
  std::uint32_t minimum = 0;
  std::optional<std::uint32_t> maximum;

  [[nodiscard]] core::result_t<void> validate() const;
  [[nodiscard]] bool accepts(
    std::size_t count
  ) const;

  bool operator==(
    const arity_constraint&
  ) const = default;
};

enum class operand_type_constraint {
  any,
  same_as_result,
};

struct operation_signature {
  arity_constraint arity;
  operand_type_constraint operand_types = operand_type_constraint::same_as_result;

  [[nodiscard]] core::result_t<void> validate_definition() const;
  [[nodiscard]] core::result_t<void> validate_application(
    const value_type& result_type,
    std::span<const value_type> operand_types,
    const operation_attributes& attributes,
    const operation_attribute_schema& attribute_schema
  ) const;

  bool operator==(
    const operation_signature&
  ) const = default;
};

} // namespace sivra::ir
