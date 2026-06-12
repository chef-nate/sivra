#pragma once

#include "machine_location.hpp"

#include <sivra/ir/value_type.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace sivra::program {

enum class write_behavior {
  full_replacement,
  preserve_unwritten,
  merge_old_destination,
  memory_store,
  architecture_state,
  unknown,
};

enum class lane_operand_role {
  old_destination,
  source,
  immediate,
};

struct lane_operand_ref {
  lane_operand_role role = lane_operand_role::source;
  std::size_t operand_index = 0;
  std::uint32_t lane = 0;

  auto operator<=>(
    const lane_operand_ref&
  ) const = default;
};

enum class lane_operation {
  zero,
  copy,
  add_f32,
  subtract_f32,
  multiply_f32,
  divide_f32,
  minimum_f32,
  maximum_f32,
  sqrt_f32,
  reciprocal_f32,
  reciprocal_sqrt_f32,
  bit_and,
  bit_and_not,
  bit_or,
  bit_xor,
};

struct lane_expression {
  lane_operation operation = lane_operation::copy;
  std::vector<lane_operand_ref> inputs;

  bool operator==(
    const lane_expression&
  ) const = default;
};

struct vector_value {
  ir::value_type type;
  std::vector<lane_expression> lanes;

  bool operator==(
    const vector_value&
  ) const = default;
};

using semantic_value = std::variant<vector_value>;

struct semantic_read {
  machine_location location;
  std::size_t operand_index = 0;
  std::string role;

  bool operator==(
    const semantic_read&
  ) const = default;
};

struct semantic_write {
  machine_location destination;
  semantic_value value;
  write_behavior behavior = write_behavior::full_replacement;

  bool operator==(
    const semantic_write&
  ) const = default;
};

struct memory_read_effect {
  memory_operand address;
  std::uint32_t width = 0;

  bool operator==(
    const memory_read_effect&
  ) const = default;
};

struct memory_write_effect {
  memory_operand address;
  semantic_value value;
  std::uint32_t width = 0;

  bool operator==(
    const memory_write_effect&
  ) const = default;
};

struct state_transition_effect {
  std::string state_class;
  std::string detail;

  bool operator==(
    const state_transition_effect&
  ) const = default;
};

struct control_effect {
  std::string kind;

  bool operator==(
    const control_effect&
  ) const = default;
};

using semantic_effect = std::variant<
  semantic_read,
  semantic_write,
  memory_read_effect,
  memory_write_effect,
  state_transition_effect,
  control_effect
>;

struct instruction_semantics {
  instruction_form_id form;
  std::vector<semantic_effect> effects;
  bool unsupported = false;
  std::string unsupported_reason;
};

} // namespace sivra::program
