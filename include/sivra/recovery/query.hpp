#pragma once

#include <sivra/ir/value_type.hpp>
#include <sivra/program/instruction.hpp>
#include <sivra/program/machine_location.hpp>

namespace sivra::recovery {

struct recovery_query {
  program::machine_location location;
  program::program_point point;
  ir::value_type expected_type = ir::value_type::unknown();

  bool operator==(
    const recovery_query&
  ) const = default;
};

} // namespace sivra::recovery
