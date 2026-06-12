#pragma once

#include "instruction.hpp"

#include <sivra/core/result.hpp>

#include <span>

namespace sivra::program {

class decoded_program {
public:
  [[nodiscard]] architecture_id architecture() const;
  [[nodiscard]] std::span<const decoded_function> functions() const;
  [[nodiscard]] std::span<const basic_block> blocks() const;
  [[nodiscard]] std::span<const decoded_instruction> instructions() const;
  [[nodiscard]] const decoded_function& function(
    function_id id
  ) const;
  [[nodiscard]] const basic_block& block(
    basic_block_id id
  ) const;
  [[nodiscard]] const decoded_instruction& instruction(
    instruction_id id
  ) const;
  [[nodiscard]] core::owner_token owner() const;
  [[nodiscard]] core::result_t<void> validate() const;

private:
  friend class decoded_program_builder;

  decoded_program(
    core::owner_token owner,
    architecture_id architecture,
    std::vector<decoded_function> functions,
    std::vector<basic_block> blocks,
    std::vector<decoded_instruction> instructions
  );

  core::owner_token m_owner;
  architecture_id m_architecture;
  std::vector<decoded_function> m_functions;
  std::vector<basic_block> m_blocks;
  std::vector<decoded_instruction> m_instructions;
};

class decoded_program_builder {
public:
  explicit decoded_program_builder(
    architecture_id architecture
  );

  [[nodiscard]] core::result_t<function_id> add_function(
    std::string name,
    std::uint64_t address = 0
  );
  [[nodiscard]] core::result_t<basic_block_id> add_block(
    function_id function
  );
  [[nodiscard]] core::result_t<instruction_id> add_instruction(
    basic_block_id block,
    instruction_form_id form,
    std::vector<operand> operands,
    std::uint64_t address = 0
  );
  [[nodiscard]] core::result_t<decoded_program> freeze() &&;
  [[nodiscard]] core::owner_token owner() const;

private:
  core::owner_token m_owner;
  architecture_id m_architecture;
  std::vector<decoded_function> m_functions;
  std::vector<basic_block> m_blocks;
  std::vector<decoded_instruction> m_instructions;
};

} // namespace sivra::program
