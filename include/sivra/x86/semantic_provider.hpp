#pragma once

#include "instruction_catalogue.hpp"
#include "register.hpp"

#include <sivra/program/semantic_provider.hpp>

#include <memory>

namespace sivra::x86 {

class semantic_provider final : public program::semantic_provider {
public:
  semantic_provider();

  [[nodiscard]] program::architecture_id architecture() const override;
  [[nodiscard]] program::architecture_profile_id profile() const override;
  [[nodiscard]] const program::register_definition& register_definition(
    program::register_id id
  ) const override;
  [[nodiscard]] const program::instruction_form_definition& form(
    program::instruction_form_id id
  ) const override;
  [[nodiscard]] core::result_t<program::instruction_semantics> semantics(
    const program::decoded_instruction& instruction
  ) const override;
  [[nodiscard]] program::location_relation relate(
    const program::machine_location& lhs,
    const program::machine_location& rhs
  ) const override;

  [[nodiscard]] const register_catalogue& registers() const;
  [[nodiscard]] const program::instruction_catalogue& instructions() const;
  [[nodiscard]] const builtin_instruction_ids& builtin_ids() const;

private:
  std::shared_ptr<const register_catalogue> m_registers;
  builtin_instruction_catalogue m_instructions;
};

} // namespace sivra::x86
