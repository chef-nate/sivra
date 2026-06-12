#pragma once

#include "id.hpp"

#include <sivra/core/result.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sivra::program {

enum class operand_constraint_kind {
  xmm_register,
  xmm_or_memory,
  memory,
  immediate,
};

enum class operand_access {
  read,
  write,
  read_write,
};

struct operand_constraint {
  operand_constraint_kind kind = operand_constraint_kind::xmm_register;
  operand_access access = operand_access::read;
  std::uint32_t width = 0;
  std::optional<std::uint32_t> immediate_width;
};

struct instruction_form_definition {
  instruction_form_id id;
  std::string key;
  std::string mnemonic;
  std::vector<operand_constraint> operands;
  std::string semantic_key;
};

class instruction_catalogue {
public:
  [[nodiscard]] const instruction_form_definition& form(
    instruction_form_id id
  ) const;
  [[nodiscard]] const instruction_form_definition* find(
    std::string_view key
  ) const;
  [[nodiscard]] std::span<const instruction_form_definition> forms() const;
  [[nodiscard]] core::owner_token owner() const;

private:
  friend class instruction_catalogue_builder;

  instruction_catalogue(
    core::owner_token owner,
    std::vector<instruction_form_definition> forms
  );

  core::owner_token m_owner;
  std::vector<instruction_form_definition> m_forms;
};

class instruction_catalogue_builder {
public:
  instruction_catalogue_builder();

  [[nodiscard]] core::result_t<instruction_form_id> register_form(
    std::string key,
    std::string mnemonic,
    std::vector<operand_constraint> operands,
    std::string semantic_key
  );
  [[nodiscard]] core::result_t<std::shared_ptr<const instruction_catalogue>> freeze() &&;
  [[nodiscard]] core::owner_token owner() const;

private:
  core::owner_token m_owner;
  std::vector<instruction_form_definition> m_forms;
};

} // namespace sivra::program
