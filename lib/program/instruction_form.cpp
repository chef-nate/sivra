#include <sivra/program/instruction_form.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <variant>

namespace sivra::program {

namespace {

core::result_t<void> validate_operand(
  const operand_constraint& constraint,
  const operand& value
) {
  switch (constraint.kind) {
  case operand_constraint_kind::register_operand:
    if (!std::holds_alternative<register_operand>(value)) {
      return core::fail<void>(
        "program.instruction_form.invalid_operand", "instruction operand must be a register"
      );
    }
    break;
  case operand_constraint_kind::register_or_memory:
    if (!std::holds_alternative<register_operand>(value) &&
        !std::holds_alternative<memory_operand>(value)) {
      return core::fail<void>(
        "program.instruction_form.invalid_operand",
        "instruction operand must be a register or memory operand"
      );
    }
    break;
  case operand_constraint_kind::memory:
    if (!std::holds_alternative<memory_operand>(value)) {
      return core::fail<void>(
        "program.instruction_form.invalid_operand", "instruction operand must be memory"
      );
    }
    break;
  case operand_constraint_kind::immediate:
    if (!std::holds_alternative<immediate_operand>(value)) {
      return core::fail<void>(
        "program.instruction_form.invalid_operand", "instruction operand must be an immediate"
      );
    }
    break;
  }

  if (const auto* reg = std::get_if<register_operand>(&value)) {
    if (auto validated = reg->slice.validate(); !validated.has_value()) {
      return std::unexpected(std::move(validated.error()));
    }
    if (constraint.width != 0 && reg->slice.width != constraint.width) {
      return core::fail<void>(
        "program.instruction_form.invalid_operand_width",
        "register operand width does not match instruction form"
      );
    }
  }
  if (const auto* memory = std::get_if<memory_operand>(&value);
      memory != nullptr && constraint.width != 0 && memory->width != constraint.width) {
    return core::fail<void>(
      "program.instruction_form.invalid_operand_width",
      "memory operand width does not match instruction form"
    );
  }
  if (const auto* immediate = std::get_if<immediate_operand>(&value)) {
    const auto expected_width = constraint.immediate_width.value_or(constraint.width);
    if (expected_width != 0 && immediate->width != expected_width) {
      return core::fail<void>(
        "program.instruction_form.invalid_operand_width",
        "immediate operand width does not match instruction form"
      );
    }
  }
  return {};
}

} // namespace

core::result_t<void> validate_instruction_operands(
  const instruction_form_definition& form,
  std::span<const operand> operands
) {
  if (form.operands.size() != operands.size()) {
    return core::fail<void>(
      "program.instruction_form.invalid_operand_count",
      "instruction operand count does not match instruction form"
    );
  }
  for (std::size_t index = 0; index < form.operands.size(); ++index) {
    if (auto validated = validate_operand(form.operands[index], operands[index]);
        !validated.has_value()) {
      return std::unexpected(std::move(validated.error()));
    }
  }
  return {};
}

instruction_catalogue::instruction_catalogue(
  core::owner_token owner,
  std::vector<instruction_form_definition> forms
)
    : m_owner(owner),
      m_forms(std::move(forms)) {
}

const instruction_form_definition& instruction_catalogue::form(
  instruction_form_id id
) const {
  if (id.owner() != m_owner || id.index() >= m_forms.size()) {
    throw std::out_of_range("instruction_form_id does not belong to this catalogue");
  }
  return m_forms[id.index()];
}

const instruction_form_definition* instruction_catalogue::find(
  std::string_view key
) const {
  const auto found =
    std::ranges::find(m_forms, key, [](const auto& form) { return std::string_view(form.key); });
  return found == m_forms.end() ? nullptr : &*found;
}

std::span<const instruction_form_definition> instruction_catalogue::forms() const {
  return m_forms;
}

core::owner_token instruction_catalogue::owner() const {
  return m_owner;
}

instruction_catalogue_builder::instruction_catalogue_builder()
    : m_owner(core::owner_token_source::next()) {
}

core::result_t<instruction_form_id> instruction_catalogue_builder::register_form(
  std::string key,
  std::string mnemonic,
  std::vector<operand_constraint> operands,
  std::string semantic_key
) {
  if (key.empty() || mnemonic.empty() || semantic_key.empty()) {
    return core::fail<instruction_form_id>(
      "program.instruction_form.invalid",
      "instruction forms require a key, mnemonic, and semantic key"
    );
  }
  if (std::ranges::any_of(m_forms, [&](const auto& existing) { return existing.key == key; })) {
    return core::fail<instruction_form_id>(
      "program.instruction_form.duplicate", "instruction form key is already registered"
    );
  }
  const auto id =
    instruction_form_id::unsafe_from_index(static_cast<std::uint32_t>(m_forms.size()), m_owner);
  m_forms.push_back(
    {
      .id = id,
      .key = std::move(key),
      .mnemonic = std::move(mnemonic),
      .operands = std::move(operands),
      .semantic_key = std::move(semantic_key),
    }
  );
  return id;
}

core::result_t<std::shared_ptr<const instruction_catalogue>>
instruction_catalogue_builder::freeze() && {
  return std::shared_ptr<const instruction_catalogue>(
    new instruction_catalogue(m_owner, std::move(m_forms))
  );
}

core::owner_token instruction_catalogue_builder::owner() const {
  return m_owner;
}

} // namespace sivra::program
