#pragma once

#include "operand.hpp"

#include <variant>

namespace sivra::program {

struct register_slice {
  register_id reg;
  bit_range bits;
  std::optional<lane_descriptor> lane;

  bool operator==(
    const register_slice&
  ) const = default;
};

struct memory_location {
  memory_operand address;

  bool operator==(
    const memory_location&
  ) const = default;
};

struct flags_location {
  std::string key;

  bool operator==(
    const flags_location&
  ) const = default;
};

using machine_location = std::variant<register_slice, memory_location, flags_location>;

enum class location_relation {
  disjoint,
  equal,
  contains,
  contained_by,
  overlaps,
  unknown,
};

} // namespace sivra::program
