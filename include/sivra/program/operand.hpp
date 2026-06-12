#pragma once

#include "id.hpp"

#include <sivra/core/result.hpp>
#include <sivra/core/source_location.hpp>
#include <sivra/ir/value_type.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace sivra::program {

struct bit_range {
  std::uint32_t offset = 0;
  std::uint32_t width = 0;

  [[nodiscard]] std::uint32_t end() const;
  [[nodiscard]] bool contains(
    bit_range other
  ) const;
  [[nodiscard]] bool overlaps(
    bit_range other
  ) const;
  [[nodiscard]] core::result_t<void> validate() const;

  auto operator<=>(
    const bit_range&
  ) const = default;
};

struct lane_descriptor {
  std::uint32_t index = 0;
  std::uint32_t element_width = 0;
  std::uint32_t lane_count = 0;

  auto operator<=>(
    const lane_descriptor&
  ) const = default;
};

struct register_operand {
  register_id reg;
  bit_range slice;
  std::optional<lane_descriptor> lane;
  std::optional<ir::value_type> type_hint;

  bool operator==(
    const register_operand&
  ) const = default;
};

struct immediate_operand {
  std::uint64_t bits = 0;
  std::uint32_t width = 0;
  bool signed_hint = false;

  auto operator<=>(
    const immediate_operand&
  ) const = default;
};

struct memory_operand {
  std::optional<register_id> base;
  std::int64_t displacement = 0;
  std::uint32_t width = 0;

  bool operator==(
    const memory_operand&
  ) const = default;
};

struct relative_target_operand {
  std::int64_t displacement = 0;

  auto operator<=>(
    const relative_target_operand&
  ) const = default;
};

struct unsupported_operand {
  std::string reason;
  std::optional<core::source_span> source;

  bool operator==(
    const unsupported_operand&
  ) const = default;
};

using operand = std::variant<
  register_operand,
  immediate_operand,
  memory_operand,
  relative_target_operand,
  unsupported_operand
>;

} // namespace sivra::program
