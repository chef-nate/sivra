#pragma once

#include "memory.hpp"

#include <sivra/core/result.hpp>
#include <sivra/program/decoded_program.hpp>
#include <sivra/program/semantic_effect.hpp>
#include <sivra/program/semantic_provider.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace sivra::recovery {

struct definition_record {
  program::instruction_id instruction;
  program::program_point before;
  program::program_point after;
  std::size_t effect_index = 0;
  program::semantic_write write;
};

class state_index {
public:
  [[nodiscard]] std::span<const definition_record> definitions() const;
  [[nodiscard]] std::span<const program::semantic_effect> effects(
    program::instruction_id instruction
  ) const;
  [[nodiscard]] std::span<const program::basic_block_id> predecessors(
    program::basic_block_id block
  ) const;
  [[nodiscard]] std::span<const program::basic_block_id> successors(
    program::basic_block_id block
  ) const;
  [[nodiscard]] std::span<const memory_transition> memory_transitions() const;
  [[nodiscard]] memory_version memory_before(
    program::program_point point
  ) const;
  [[nodiscard]] memory_version memory_after(
    program::program_point point
  ) const;
  [[nodiscard]] core::owner_token owner() const;

private:
  friend class state_index_builder;

  struct instruction_semantics_record {
    std::optional<program::instruction_id> instruction;
    program::instruction_semantics semantics;
  };

  core::owner_token m_owner;
  core::owner_token m_program_owner;
  std::vector<definition_record> m_definitions;
  std::vector<std::optional<instruction_semantics_record>> m_instruction_semantics;
  std::vector<std::vector<program::basic_block_id>> m_predecessors;
  std::vector<std::vector<program::basic_block_id>> m_successors;
  std::vector<memory_transition> m_memory_transitions;
  std::vector<memory_version> m_memory_before;
  std::vector<memory_version> m_memory_after;
};

class state_index_builder {
public:
  [[nodiscard]] static core::result_t<state_index> build(
    const program::decoded_program& program,
    const program::semantic_provider& semantics
  );
};

} // namespace sivra::recovery
