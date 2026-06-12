#include <sivra/program/instruction_form.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sivra::program {

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
