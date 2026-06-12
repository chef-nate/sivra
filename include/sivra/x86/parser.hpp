#pragma once

#include "token.hpp"

#include <sivra/core/result.hpp>

#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace sivra::x86 {

struct unresolved_register_operand {
  std::string name;
  core::source_span source;
};

struct unresolved_immediate_operand {
  std::uint64_t value = 0;
  core::source_span source;
};

struct unresolved_memory_operand {
  std::string base;
  std::int64_t displacement = 0;
  core::source_span source;
};

using unresolved_operand = std::
  variant<unresolved_register_operand, unresolved_immediate_operand, unresolved_memory_operand>;

struct unresolved_instruction {
  std::string mnemonic;
  std::vector<unresolved_operand> operands;
  core::source_span source;
};

class parser {
public:
  [[nodiscard]] core::result_t<std::vector<unresolved_instruction>> parse(
    std::span<const token> tokens
  ) const;
};

} // namespace sivra::x86
