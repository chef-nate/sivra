#pragma once

#include "decoded_program.hpp"
#include "instruction_form.hpp"
#include "semantic_effect.hpp"

#include <sivra/core/result.hpp>

namespace sivra::program {

struct register_definition {
  register_id id;
  std::string key;
  std::string name;
  std::uint32_t width = 0;
  std::optional<register_id> parent;
  bit_range parent_range;
};

class semantic_provider {
public:
  virtual ~semantic_provider() = default;

  [[nodiscard]] virtual architecture_id architecture() const = 0;
  [[nodiscard]] virtual architecture_profile_id profile() const = 0;
  [[nodiscard]] virtual const register_definition& register_definition(
    register_id id
  ) const = 0;
  [[nodiscard]] virtual const instruction_form_definition& form(
    instruction_form_id id
  ) const = 0;
  [[nodiscard]] virtual core::result_t<instruction_semantics> semantics(
    const decoded_instruction& instruction
  ) const = 0;
  [[nodiscard]] virtual location_relation relate(
    const machine_location& lhs,
    const machine_location& rhs
  ) const = 0;
};

} // namespace sivra::program
