#include <sivra/recovery/state_index.hpp>

#include <algorithm>
#include <utility>

namespace sivra::recovery {

std::span<const definition_record> state_index::definitions() const {
  return m_definitions;
}

std::span<const program::semantic_effect> state_index::effects(
  program::instruction_id instruction
) const {
  if (instruction.owner() != m_program_owner ||
      instruction.index() >= m_instruction_semantics.size() ||
      !m_instruction_semantics[instruction.index()].has_value()) {
    return {};
  }
  return m_instruction_semantics[instruction.index()]->semantics.effects;
}

std::span<const program::basic_block_id> state_index::predecessors(
  program::basic_block_id block
) const {
  if (block.owner() != m_program_owner || block.index() >= m_predecessors.size()) {
    return {};
  }
  return m_predecessors[block.index()];
}

std::span<const program::basic_block_id> state_index::successors(
  program::basic_block_id block
) const {
  if (block.owner() != m_program_owner || block.index() >= m_successors.size()) {
    return {};
  }
  return m_successors[block.index()];
}

std::span<const memory_transition> state_index::memory_transitions() const {
  return m_memory_transitions;
}

memory_version state_index::memory_before(
  program::program_point point
) const {
  if (point.instruction.owner() != m_program_owner ||
      point.instruction.index() >= m_memory_before.size()) {
    return memory_version::unsafe_from_index(0, m_owner);
  }
  return m_memory_before[point.instruction.index()];
}

memory_version state_index::memory_after(
  program::program_point point
) const {
  if (point.instruction.owner() != m_program_owner ||
      point.instruction.index() >= m_memory_after.size()) {
    return memory_version::unsafe_from_index(0, m_owner);
  }
  return m_memory_after[point.instruction.index()];
}

core::owner_token state_index::owner() const {
  return m_owner;
}

core::result_t<state_index> state_index_builder::build(
  const program::decoded_program& program,
  const program::semantic_provider& semantics
) {
  if (auto validated = program.validate(); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }

  state_index index;
  index.m_owner = core::owner_token_source::next();
  index.m_program_owner = program.owner();
  index.m_instruction_semantics.resize(program.instructions().size());
  index.m_predecessors.resize(program.blocks().size());
  index.m_successors.resize(program.blocks().size());
  for (const auto& block : program.blocks()) {
    index.m_predecessors[block.id.index()] = block.predecessors;
    index.m_successors[block.id.index()] = block.successors;
  }
  const auto initial_memory = memory_version::unsafe_from_index(0, index.m_owner);
  index.m_memory_before.resize(program.instructions().size(), initial_memory);
  index.m_memory_after.resize(program.instructions().size(), initial_memory);
  auto next_memory = std::uint32_t{1};
  for (const auto& block : program.blocks()) {
    auto current_memory = initial_memory;
    for (const auto instruction_id : block.instructions) {
      const auto& instruction = program.instruction(instruction_id);
      auto instruction_semantics = semantics.semantics(instruction);
      if (!instruction_semantics.has_value()) {
        return std::unexpected(std::move(instruction_semantics.error()));
      }

      index.m_memory_before[instruction.id.index()] = current_memory;
      for (std::size_t effect_index = 0; effect_index < instruction_semantics->effects.size();
           ++effect_index) {
        const auto& effect = instruction_semantics->effects[effect_index];
        if (const auto* write = std::get_if<program::semantic_write>(&effect)) {
          index.m_definitions.push_back(
            definition_record{
              .instruction = instruction.id,
              .before =
                {
                  .block = block.id,
                  .instruction = instruction.id,
                  .phase = program::point_phase::before,
                },
              .after =
                {
                  .block = block.id,
                  .instruction = instruction.id,
                  .phase = program::point_phase::after,
                },
              .effect_index = effect_index,
              .write = *write,
            }
          );
        } else if (const auto* write = std::get_if<program::memory_write_effect>(&effect)) {
          const auto after = memory_version::unsafe_from_index(next_memory++, index.m_owner);
          index.m_memory_transitions.push_back(
            memory_transition{
              .instruction = instruction.id,
              .block = block.id,
              .before = current_memory,
              .after = after,
              .address = write->address,
              .width = write->width,
            }
          );
          current_memory = after;
        }
      }
      index.m_memory_after[instruction.id.index()] = current_memory;
      index.m_instruction_semantics[instruction.id.index()] =
        state_index::instruction_semantics_record{
          .instruction = instruction.id,
          .semantics = std::move(*instruction_semantics),
      };
    }
  }

  return index;
}

} // namespace sivra::recovery
