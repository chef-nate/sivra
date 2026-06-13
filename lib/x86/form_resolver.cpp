#include <sivra/x86/form_resolver.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace {

bool is_memory(
  const sivra::x86::unresolved_operand& operand
) {
  return std::holds_alternative<sivra::x86::unresolved_memory_operand>(operand);
}

bool matches_shape(
  const sivra::x86::unresolved_operand& operand,
  const sivra::program::operand_constraint& constraint
) {
  using enum sivra::program::operand_constraint_kind;
  switch (constraint.kind) {
  case register_operand:
    return std::holds_alternative<sivra::x86::unresolved_register_operand>(operand);
  case register_or_memory:
    return std::holds_alternative<sivra::x86::unresolved_register_operand>(operand) ||
           std::holds_alternative<sivra::x86::unresolved_memory_operand>(operand);
  case memory:
    return std::holds_alternative<sivra::x86::unresolved_memory_operand>(operand);
  case immediate:
    return std::holds_alternative<sivra::x86::unresolved_immediate_operand>(operand);
  }
  return false;
}

bool form_matches(
  const sivra::program::instruction_form_definition& form,
  const sivra::x86::unresolved_instruction& instruction
) {
  if (form.mnemonic != instruction.mnemonic ||
      form.operands.size() != instruction.operands.size()) {
    return false;
  }
  for (std::size_t index = 0; index < form.operands.size(); ++index) {
    if (!matches_shape(instruction.operands[index], form.operands[index])) {
      return false;
    }
  }
  return true;
}

} // namespace

namespace sivra::x86 {

form_resolver::form_resolver(
  std::shared_ptr<const register_catalogue> registers,
  std::shared_ptr<const program::instruction_catalogue> instructions
)
    : m_registers(std::move(registers)),
      m_instructions(std::move(instructions)) {
}

core::result_t<program::operand> form_resolver::resolve_operand(
  const unresolved_operand& operand,
  const program::operand_constraint& constraint
) const {
  if (const auto* reg = std::get_if<unresolved_register_operand>(&operand)) {
    const auto* info = m_registers->find(reg->name);
    if (info == nullptr) {
      return core::fail<program::operand>(
        "x86.resolver.unknown_register", "x86 resolver could not resolve register name"
      );
    }
    if (info->bank != register_bank::simd) {
      return core::fail<program::operand>(
        "x86.resolver.invalid_register_class", "x86 resolver expected an XMM register"
      );
    }
    if (!constraint.register_class.empty() && constraint.register_class != "x86.xmm") {
      return core::fail<program::operand>(
        "x86.resolver.invalid_register_class", "x86 resolver found an unsupported register class"
      );
    }
    return program::operand(
      program::register_operand{
        .reg = info->definition.id,
        .slice = {.offset = 0, .width = constraint.width},
        .lane = constraint.width == 32
                  ? std::optional<program::lane_descriptor>(program::lane_descriptor{
                      .index = 0, .element_width = 32, .lane_count = 4})
                  : std::nullopt,
        .type_hint = constraint.width == 32 ? std::optional(ir::value_type::f32()) : std::nullopt,
      }
    );
  }
  if (const auto* immediate = std::get_if<unresolved_immediate_operand>(&operand)) {
    if (constraint.kind != program::operand_constraint_kind::immediate) {
      return core::fail<program::operand>(
        "x86.resolver.invalid_immediate", "x86 resolver did not expect an immediate operand"
      );
    }
    return program::operand(
      program::immediate_operand{
        .bits = immediate->value,
        .width = constraint.immediate_width.value_or(constraint.width),
      }
    );
  }
  if (const auto* memory = std::get_if<unresolved_memory_operand>(&operand)) {
    const auto* base = m_registers->find(memory->base);
    if (base == nullptr || base->bank != register_bank::gpr) {
      return core::fail<program::operand>(
        "x86.resolver.invalid_memory_base", "x86 resolver expected a GPR memory base register"
      );
    }
    return program::operand(
      program::memory_operand{
        .base = base->definition.id,
        .displacement = memory->displacement,
        .width = constraint.width,
      }
    );
  }
  return core::fail<program::operand>(
    "x86.resolver.unsupported_operand", "x86 resolver encountered an unsupported operand"
  );
}

core::result_t<program::decoded_program> form_resolver::resolve(
  std::span<const unresolved_instruction> instructions,
  resolution_context context
) const {
  program::decoded_program_builder builder(program::architecture_id("x86"));
  auto function = builder.add_function(std::move(context.function_name), context.base_address);
  if (!function.has_value()) {
    return std::unexpected(std::move(function.error()));
  }
  auto block = builder.add_block(*function);
  if (!block.has_value()) {
    return std::unexpected(std::move(block.error()));
  }

  for (std::size_t instruction_index = 0; instruction_index < instructions.size();
       ++instruction_index) {
    const auto& instruction = instructions[instruction_index];
    std::vector<const program::instruction_form_definition*> candidates;
    for (const auto& form : m_instructions->forms()) {
      if (form_matches(form, instruction)) {
        candidates.push_back(&form);
      }
    }
    if (candidates.empty()) {
      return core::fail<program::decoded_program>(
        "x86.resolver.unsupported_form", "x86 resolver could not match an instruction form"
      );
    }

    const auto* selected = candidates.front();
    if (candidates.size() > 1) {
      const auto first_is_memory = is_memory(instruction.operands.front());
      const auto found = std::ranges::find_if(candidates, [&](const auto* candidate) {
        const auto& first_constraint = candidate->operands.front();
        return first_is_memory ? first_constraint.kind == program::operand_constraint_kind::memory
                               : first_constraint.kind != program::operand_constraint_kind::memory;
      });
      if (found != candidates.end()) {
        selected = *found;
      }
    }

    std::vector<program::operand> operands;
    operands.reserve(instruction.operands.size());
    for (std::size_t operand_index = 0; operand_index < instruction.operands.size();
         ++operand_index) {
      auto resolved =
        resolve_operand(instruction.operands[operand_index], selected->operands[operand_index]);
      if (!resolved.has_value()) {
        return std::unexpected(std::move(resolved.error()));
      }
      operands.push_back(std::move(*resolved));
    }

    auto added = builder.add_instruction(
      *block,
      selected->id,
      std::move(operands),
      context.base_address + static_cast<std::uint64_t>(instruction_index),
      instruction.source
    );
    if (!added.has_value()) {
      return std::unexpected(std::move(added.error()));
    }
  }

  return std::move(builder).freeze();
}

} // namespace sivra::x86
