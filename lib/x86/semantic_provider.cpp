#include <sivra/x86/semantic_provider.hpp>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>

namespace {

using sivra::program::lane_expression;
using sivra::program::lane_operand_ref;
using sivra::program::lane_operand_role;
using sivra::program::lane_operation;

bool is_memory_operand(
  const sivra::program::operand& operand
) {
  return std::holds_alternative<sivra::program::memory_operand>(operand);
}

sivra::program::register_slice register_destination(
  const sivra::program::operand& operand
) {
  const auto& reg = std::get<sivra::program::register_operand>(operand);
  return {
    .reg = reg.reg,
    .bits = {.offset = 0, .width = 128},
  };
}

lane_operand_ref old_lane(
  std::uint32_t lane
) {
  return {
    .role = lane_operand_role::old_destination,
    .lane = lane,
  };
}

lane_operand_ref source_lane(
  std::size_t operand,
  std::uint32_t lane
) {
  return {
    .role = lane_operand_role::source,
    .operand_index = operand,
    .lane = lane,
  };
}

lane_expression copy(
  lane_operand_ref input
) {
  return {
    .operation = lane_operation::copy,
    .inputs = {input},
  };
}

lane_expression zero() {
  return {
    .operation = lane_operation::zero,
  };
}

lane_expression unary(
  lane_operation operation,
  std::uint32_t lane
) {
  return {
    .operation = operation,
    .inputs = {source_lane(1, lane)},
  };
}

lane_expression binary(
  lane_operation operation,
  std::uint32_t lane
) {
  return {
    .operation = operation,
    .inputs = {old_lane(lane), source_lane(1, lane)},
  };
}

sivra::program::vector_value packed_binary(
  lane_operation operation
) {
  std::vector<lane_expression> lanes;
  lanes.reserve(4);
  for (std::uint32_t lane = 0; lane < 4; ++lane) {
    lanes.push_back(binary(operation, lane));
  }
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = std::move(lanes),
  };
}

sivra::program::vector_value scalar_binary(
  lane_operation operation
) {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = {binary(operation, 0), copy(old_lane(1)), copy(old_lane(2)), copy(old_lane(3))},
  };
}

sivra::program::vector_value packed_unary(
  lane_operation operation
) {
  std::vector<lane_expression> lanes;
  lanes.reserve(4);
  for (std::uint32_t lane = 0; lane < 4; ++lane) {
    lanes.push_back(unary(operation, lane));
  }
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = std::move(lanes),
  };
}

sivra::program::vector_value scalar_unary(
  lane_operation operation
) {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = {unary(operation, 0), copy(old_lane(1)), copy(old_lane(2)), copy(old_lane(3))},
  };
}

sivra::program::vector_value full_copy() {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = {copy(source_lane(1, 0)),
              copy(source_lane(1, 1)),
              copy(source_lane(1, 2)),
              copy(source_lane(1, 3))},
  };
}

sivra::program::vector_value scalar_store_copy() {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = {copy(source_lane(1, 0))},
  };
}

sivra::program::vector_value movss_load(
  bool memory_source
) {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes =
      memory_source
        ? std::vector<lane_expression>{copy(source_lane(1, 0)), zero(), zero(), zero()}
        : std::vector<lane_expression>{
            copy(source_lane(1, 0)),
            copy(old_lane(1)),
            copy(old_lane(2)),
            copy(old_lane(3)),
          },
  };
}

sivra::program::vector_value shufps(
  std::uint8_t selector
) {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes =
      {
        copy(old_lane(selector & 0x3U)),
        copy(old_lane((selector >> 2U) & 0x3U)),
        copy(source_lane(1, (selector >> 4U) & 0x3U)),
        copy(source_lane(1, (selector >> 6U) & 0x3U)),
      },
  };
}

sivra::program::vector_value unpcklps() {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = {copy(old_lane(0)),
              copy(source_lane(1, 0)),
              copy(old_lane(1)),
              copy(source_lane(1, 1))},
  };
}

sivra::program::vector_value unpckhps() {
  return {
    .type = sivra::ir::value_type::f32(),
    .lanes = {copy(old_lane(2)),
              copy(source_lane(1, 2)),
              copy(old_lane(3)),
              copy(source_lane(1, 3))},
  };
}

sivra::program::semantic_write register_write(
  const sivra::program::decoded_instruction& instruction,
  sivra::program::vector_value value,
  sivra::program::write_behavior behavior
) {
  return {
    .destination = register_destination(instruction.operands.front()),
    .value = std::move(value),
    .behavior = behavior,
  };
}

std::optional<sivra::program::memory_read_effect> source_memory_read(
  const sivra::program::decoded_instruction& instruction
) {
  if (instruction.operands.size() <= 1) {
    return std::nullopt;
  }
  const auto* memory = std::get_if<sivra::program::memory_operand>(&instruction.operands[1]);
  if (memory == nullptr) {
    return std::nullopt;
  }
  return sivra::program::memory_read_effect{
    .address = *memory,
    .width = memory->width,
  };
}

} // namespace

namespace sivra::x86 {

semantic_provider::semantic_provider()
    : m_registers(builtin_register_catalogue()),
      m_instructions(builtin_sse1_instruction_catalogue()) {
}

program::architecture_id semantic_provider::architecture() const {
  return program::architecture_id("x86");
}

program::architecture_profile_id semantic_provider::profile() const {
  return program::architecture_profile_id("sse1");
}

const program::register_definition& semantic_provider::register_definition(
  program::register_id id
) const {
  return m_registers->at(id).definition;
}

const program::instruction_form_definition& semantic_provider::form(
  program::instruction_form_id id
) const {
  return m_instructions.catalogue->form(id);
}

core::result_t<program::instruction_semantics> semantic_provider::semantics(
  const program::decoded_instruction& instruction
) const {
  if (instruction.form.owner() != m_instructions.catalogue->owner()) {
    return core::fail<program::instruction_semantics>(
      "x86.semantic_provider.foreign_form", "instruction form does not belong to the x86 catalogue"
    );
  }

  const auto& ids = m_instructions.ids;
  const auto scalar_source_is_memory =
    instruction.operands.size() > 1 && is_memory_operand(instruction.operands[1]);
  const auto emit_register = [&](
                               program::vector_value value, program::write_behavior behavior
                             ) -> program::instruction_semantics {
    std::vector<program::semantic_effect> effects;
    if (auto read = source_memory_read(instruction); read.has_value()) {
      effects.emplace_back(*read);
    }
    effects.emplace_back(register_write(instruction, std::move(value), behavior));
    return {
      .form = instruction.form,
      .effects = std::move(effects),
    };
  };

  if (instruction.form == ids.addps) {
    return emit_register(
      packed_binary(lane_operation::add_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.addss) {
    return emit_register(
      scalar_binary(lane_operation::add_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.subps) {
    return emit_register(
      packed_binary(lane_operation::subtract_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.subss) {
    return emit_register(
      scalar_binary(lane_operation::subtract_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.mulps) {
    return emit_register(
      packed_binary(lane_operation::multiply_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.mulss) {
    return emit_register(
      scalar_binary(lane_operation::multiply_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.divps) {
    return emit_register(
      packed_binary(lane_operation::divide_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.divss) {
    return emit_register(
      scalar_binary(lane_operation::divide_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.minps) {
    return emit_register(
      packed_binary(lane_operation::minimum_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.minss) {
    return emit_register(
      scalar_binary(lane_operation::minimum_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.maxps) {
    return emit_register(
      packed_binary(lane_operation::maximum_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.maxss) {
    return emit_register(
      scalar_binary(lane_operation::maximum_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.sqrtps) {
    return emit_register(
      packed_unary(lane_operation::sqrt_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.sqrtss) {
    return emit_register(
      scalar_unary(lane_operation::sqrt_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.rcpps) {
    return emit_register(
      packed_unary(lane_operation::reciprocal_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.rcpss) {
    return emit_register(
      scalar_unary(lane_operation::reciprocal_f32), program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.rsqrtps) {
    return emit_register(
      packed_unary(lane_operation::reciprocal_sqrt_f32), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.rsqrtss) {
    return emit_register(
      scalar_unary(lane_operation::reciprocal_sqrt_f32),
      program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.andps) {
    return emit_register(
      packed_binary(lane_operation::bit_and), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.andnps) {
    return emit_register(
      packed_binary(lane_operation::bit_and_not), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.orps) {
    return emit_register(
      packed_binary(lane_operation::bit_or), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.xorps) {
    return emit_register(
      packed_binary(lane_operation::bit_xor), program::write_behavior::full_replacement
    );
  }
  if (instruction.form == ids.movaps_load || instruction.form == ids.movups_load) {
    return emit_register(full_copy(), program::write_behavior::full_replacement);
  }
  if (instruction.form == ids.movss_load) {
    return emit_register(
      movss_load(scalar_source_is_memory),
      scalar_source_is_memory ? program::write_behavior::full_replacement
                              : program::write_behavior::merge_old_destination
    );
  }
  if (instruction.form == ids.movaps_store || instruction.form == ids.movups_store ||
      instruction.form == ids.movss_store) {
    const auto width = std::get<program::memory_operand>(instruction.operands.front()).width;
    return program::instruction_semantics{
      .form = instruction.form,
      .effects =
        {
          program::memory_write_effect{
            .address = std::get<program::memory_operand>(instruction.operands.front()),
            .value = instruction.form == ids.movss_store ? scalar_store_copy() : full_copy(),
            .width = width,
          },
        },
    };
  }
  if (instruction.form == ids.shufps) {
    const auto selector =
      static_cast<std::uint8_t>(std::get<program::immediate_operand>(instruction.operands[2]).bits);
    return emit_register(shufps(selector), program::write_behavior::full_replacement);
  }
  if (instruction.form == ids.unpcklps) {
    return emit_register(unpcklps(), program::write_behavior::full_replacement);
  }
  if (instruction.form == ids.unpckhps) {
    return emit_register(unpckhps(), program::write_behavior::full_replacement);
  }

  return program::instruction_semantics{
    .form = instruction.form,
    .unsupported = true,
    .unsupported_reason = "x86 semantic provider has no implementation for this form",
  };
}

program::location_relation semantic_provider::relate(
  const program::machine_location& lhs,
  const program::machine_location& rhs
) const {
  const auto* lhs_register = std::get_if<program::register_slice>(&lhs);
  const auto* rhs_register = std::get_if<program::register_slice>(&rhs);
  if (lhs_register == nullptr || rhs_register == nullptr) {
    return lhs == rhs ? program::location_relation::equal : program::location_relation::unknown;
  }
  if (lhs_register->reg != rhs_register->reg) {
    return program::location_relation::disjoint;
  }
  if (lhs_register->bits == rhs_register->bits) {
    return program::location_relation::equal;
  }
  if (lhs_register->bits.contains(rhs_register->bits)) {
    return program::location_relation::contains;
  }
  if (rhs_register->bits.contains(lhs_register->bits)) {
    return program::location_relation::contained_by;
  }
  return lhs_register->bits.overlaps(rhs_register->bits) ? program::location_relation::overlaps
                                                         : program::location_relation::disjoint;
}

const register_catalogue& semantic_provider::registers() const {
  return *m_registers;
}

const program::instruction_catalogue& semantic_provider::instructions() const {
  return *m_instructions.catalogue;
}

const builtin_instruction_ids& semantic_provider::builtin_ids() const {
  return m_instructions.ids;
}

} // namespace sivra::x86
