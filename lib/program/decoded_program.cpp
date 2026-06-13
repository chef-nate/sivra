#include <sivra/program/decoded_program.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sivra::program {

decoded_program::decoded_program(
  core::owner_token owner,
  architecture_id architecture,
  std::vector<decoded_function> functions,
  std::vector<basic_block> blocks,
  std::vector<decoded_instruction> instructions
)
    : m_owner(owner),
      m_architecture(std::move(architecture)),
      m_functions(std::move(functions)),
      m_blocks(std::move(blocks)),
      m_instructions(std::move(instructions)) {
}

architecture_id decoded_program::architecture() const {
  return m_architecture;
}

std::span<const decoded_function> decoded_program::functions() const {
  return m_functions;
}

std::span<const basic_block> decoded_program::blocks() const {
  return m_blocks;
}

std::span<const decoded_instruction> decoded_program::instructions() const {
  return m_instructions;
}

const decoded_function& decoded_program::function(
  function_id id
) const {
  if (id.owner() != m_owner || id.index() >= m_functions.size()) {
    throw std::out_of_range("function_id does not belong to this decoded_program");
  }
  return m_functions[id.index()];
}

const basic_block& decoded_program::block(
  basic_block_id id
) const {
  if (id.owner() != m_owner || id.index() >= m_blocks.size()) {
    throw std::out_of_range("basic_block_id does not belong to this decoded_program");
  }
  return m_blocks[id.index()];
}

const decoded_instruction& decoded_program::instruction(
  instruction_id id
) const {
  if (id.owner() != m_owner || id.index() >= m_instructions.size()) {
    throw std::out_of_range("instruction_id does not belong to this decoded_program");
  }
  return m_instructions[id.index()];
}

core::owner_token decoded_program::owner() const {
  return m_owner;
}

core::result_t<void> decoded_program::validate() const {
  if (m_architecture.empty()) {
    return core::fail<void>(
      "program.decoded_program.invalid_architecture", "decoded program architecture is empty"
    );
  }
  for (std::size_t index = 0; index < m_functions.size(); ++index) {
    const auto expected =
      function_id::unsafe_from_index(static_cast<std::uint32_t>(index), m_owner);
    if (m_functions[index].id != expected) {
      return core::fail<void>(
        "program.decoded_program.invalid_function", "decoded function id is not canonical"
      );
    }
    if (m_functions[index].name.empty()) {
      return core::fail<void>(
        "program.decoded_program.invalid_function", "decoded function name is empty"
      );
    }
    if (m_functions[index].blocks.empty()) {
      return core::fail<void>(
        "program.decoded_program.invalid_function", "decoded function has no basic blocks"
      );
    }
    const auto entry = m_functions[index].entry_block;
    if (entry.owner() != m_owner || entry.index() >= m_blocks.size() ||
        !std::ranges::contains(m_functions[index].blocks, entry)) {
      return core::fail<void>(
        "program.decoded_program.invalid_function",
        "decoded function entry block does not belong to the function"
      );
    }
    for (const auto block : m_functions[index].blocks) {
      if (block.owner() != m_owner || block.index() >= m_blocks.size()) {
        return core::fail<void>(
          "program.decoded_program.invalid_block_ref",
          "decoded function references a block outside the decoded program"
        );
      }
    }
  }
  std::vector<bool> referenced_instructions(m_instructions.size(), false);
  for (std::size_t index = 0; index < m_blocks.size(); ++index) {
    const auto expected =
      basic_block_id::unsafe_from_index(static_cast<std::uint32_t>(index), m_owner);
    if (m_blocks[index].id != expected) {
      return core::fail<void>(
        "program.decoded_program.invalid_block", "basic block id is not canonical"
      );
    }
    for (const auto instruction : m_blocks[index].instructions) {
      if (instruction.owner() != m_owner || instruction.index() >= m_instructions.size()) {
        return core::fail<void>(
          "program.decoded_program.invalid_instruction_ref",
          "basic block references an instruction outside the decoded program"
        );
      }
      if (referenced_instructions[instruction.index()]) {
        return core::fail<void>(
          "program.decoded_program.duplicate_instruction_ref",
          "instruction appears in more than one basic-block position"
        );
      }
      referenced_instructions[instruction.index()] = true;
    }
    for (const auto successor : m_blocks[index].successors) {
      if (successor.owner() != m_owner || successor.index() >= m_blocks.size()) {
        return core::fail<void>(
          "program.decoded_program.invalid_block_ref",
          "basic block references a successor outside the decoded program"
        );
      }
    }
    for (const auto predecessor : m_blocks[index].predecessors) {
      if (predecessor.owner() != m_owner || predecessor.index() >= m_blocks.size()) {
        return core::fail<void>(
          "program.decoded_program.invalid_block_ref",
          "basic block references a predecessor outside the decoded program"
        );
      }
    }
  }
  for (std::size_t index = 0; index < m_instructions.size(); ++index) {
    const auto expected =
      instruction_id::unsafe_from_index(static_cast<std::uint32_t>(index), m_owner);
    if (m_instructions[index].id != expected) {
      return core::fail<void>(
        "program.decoded_program.invalid_instruction", "instruction id is not canonical"
      );
    }
    if (!referenced_instructions[index]) {
      return core::fail<void>(
        "program.decoded_program.missing_instruction_ref",
        "instruction is not referenced by any basic block"
      );
    }
  }
  return {};
}

decoded_program_builder::decoded_program_builder(
  architecture_id architecture
)
    : m_owner(core::owner_token_source::next()),
      m_architecture(std::move(architecture)) {
}

core::result_t<function_id> decoded_program_builder::add_function(
  std::string name,
  std::uint64_t address
) {
  if (name.empty()) {
    return core::fail<function_id>(
      "program.decoded_program.invalid_function", "decoded function name must not be empty"
    );
  }
  const auto id =
    function_id::unsafe_from_index(static_cast<std::uint32_t>(m_functions.size()), m_owner);
  m_functions.push_back(
    {
      .id = id,
      .name = std::move(name),
      .address = address,
      .entry_block = basic_block_id::unsafe_from_index(0, m_owner),
      .blocks = {},
    }
  );
  return id;
}

core::result_t<basic_block_id> decoded_program_builder::add_block(
  function_id function
) {
  if (function.owner() != m_owner || function.index() >= m_functions.size()) {
    return core::fail<basic_block_id>(
      "program.decoded_program.invalid_function",
      "basic block parent function does not belong to the decoded program"
    );
  }
  const auto id =
    basic_block_id::unsafe_from_index(static_cast<std::uint32_t>(m_blocks.size()), m_owner);
  m_blocks.push_back({.id = id});
  auto& parent = m_functions[function.index()];
  if (parent.blocks.empty()) {
    parent.entry_block = id;
  }
  parent.blocks.push_back(id);
  return id;
}

core::result_t<instruction_id> decoded_program_builder::add_instruction(
  basic_block_id block,
  instruction_form_id form,
  std::vector<operand> operands,
  std::uint64_t address,
  std::optional<core::source_span> source
) {
  if (block.owner() != m_owner || block.index() >= m_blocks.size()) {
    return core::fail<instruction_id>(
      "program.decoded_program.invalid_block",
      "instruction parent block does not belong to the decoded program"
    );
  }
  const auto id =
    instruction_id::unsafe_from_index(static_cast<std::uint32_t>(m_instructions.size()), m_owner);
  m_instructions.push_back(
    {
      .id = id,
      .form = form,
      .address = address,
      .operands = std::move(operands),
      .source = std::move(source),
    }
  );
  m_blocks[block.index()].instructions.push_back(id);
  return id;
}

core::result_t<decoded_program> decoded_program_builder::freeze() && {
  decoded_program program(
    m_owner,
    std::move(m_architecture),
    std::move(m_functions),
    std::move(m_blocks),
    std::move(m_instructions)
  );
  if (auto validated = program.validate(); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }
  return program;
}

core::owner_token decoded_program_builder::owner() const {
  return m_owner;
}

} // namespace sivra::program
