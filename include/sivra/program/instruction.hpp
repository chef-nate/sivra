#pragma once

#include "operand.hpp"

#include <sivra/core/source_location.hpp>

#include <optional>
#include <string>
#include <vector>

namespace sivra::program {

struct decoded_instruction {
  instruction_id id;
  instruction_form_id form;
  std::uint64_t address = 0;
  std::vector<operand> operands;
  std::optional<core::source_span> source;
};

struct basic_block {
  basic_block_id id;
  std::vector<instruction_id> instructions;
  std::vector<basic_block_id> successors;
  std::vector<basic_block_id> predecessors;
};

struct decoded_function {
  function_id id;
  std::string name;
  std::uint64_t address = 0;
  basic_block_id entry_block;
  std::vector<basic_block_id> blocks;
};

enum class point_phase {
  before,
  after,
};

struct program_point {
  basic_block_id block;
  instruction_id instruction;
  point_phase phase = point_phase::before;

  auto operator<=>(
    const program_point&
  ) const = default;
};

} // namespace sivra::program
