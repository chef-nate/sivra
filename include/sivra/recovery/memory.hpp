#pragma once

#include "id.hpp"

#include <sivra/program/operand.hpp>

#include <cstdint>
#include <optional>

namespace sivra::recovery {

enum class alias_relation {
  no_alias,
  must_alias,
  may_alias,
  unknown,
};

class memory_alias_analysis {
public:
  virtual ~memory_alias_analysis() = default;

  [[nodiscard]] virtual alias_relation relate(
    const program::memory_operand& lhs,
    const program::memory_operand& rhs
  ) const = 0;
};

class conservative_memory_alias_analysis final : public memory_alias_analysis {
public:
  [[nodiscard]] alias_relation relate(
    const program::memory_operand& lhs,
    const program::memory_operand& rhs
  ) const override;
};

struct memory_transition {
  program::instruction_id instruction;
  program::basic_block_id block;
  memory_version before;
  memory_version after;
  program::memory_operand address;
  std::uint32_t width = 0;
};

} // namespace sivra::recovery
