#pragma once

#include "instruction_catalogue.hpp"
#include "parser.hpp"
#include "register.hpp"

#include <sivra/program/decoded_program.hpp>

#include <memory>
#include <span>

namespace sivra::x86 {

struct resolution_context {
  std::string function_name = "entry";
  std::uint64_t base_address = 0;
};

class form_resolver {
public:
  form_resolver(
    std::shared_ptr<const register_catalogue> registers,
    std::shared_ptr<const program::instruction_catalogue> instructions
  );

  [[nodiscard]] core::result_t<program::decoded_program> resolve(
    std::span<const unresolved_instruction> instructions,
    resolution_context context = {}
  ) const;

private:
  [[nodiscard]] core::result_t<program::operand> resolve_operand(
    const unresolved_operand& operand,
    const program::operand_constraint& constraint
  ) const;

  std::shared_ptr<const register_catalogue> m_registers;
  std::shared_ptr<const program::instruction_catalogue> m_instructions;
};

} // namespace sivra::x86
