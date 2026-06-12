#pragma once

#include <sivra/program/instruction_form.hpp>

#include <memory>

namespace sivra::x86 {

struct builtin_instruction_ids {
  program::instruction_form_id addps;
  program::instruction_form_id addss;
  program::instruction_form_id subps;
  program::instruction_form_id subss;
  program::instruction_form_id mulps;
  program::instruction_form_id mulss;
  program::instruction_form_id divps;
  program::instruction_form_id divss;
  program::instruction_form_id minps;
  program::instruction_form_id minss;
  program::instruction_form_id maxps;
  program::instruction_form_id maxss;
  program::instruction_form_id sqrtps;
  program::instruction_form_id sqrtss;
  program::instruction_form_id rcpps;
  program::instruction_form_id rcpss;
  program::instruction_form_id rsqrtps;
  program::instruction_form_id rsqrtss;
  program::instruction_form_id andps;
  program::instruction_form_id andnps;
  program::instruction_form_id orps;
  program::instruction_form_id xorps;
  program::instruction_form_id movaps_load;
  program::instruction_form_id movaps_store;
  program::instruction_form_id movups_load;
  program::instruction_form_id movups_store;
  program::instruction_form_id movss_load;
  program::instruction_form_id movss_store;
  program::instruction_form_id shufps;
  program::instruction_form_id unpcklps;
  program::instruction_form_id unpckhps;
};

struct builtin_instruction_catalogue {
  std::shared_ptr<const program::instruction_catalogue> catalogue;
  builtin_instruction_ids ids;
};

[[nodiscard]] builtin_instruction_catalogue builtin_sse1_instruction_catalogue();

} // namespace sivra::x86
