#pragma once

#include "id.hpp"

#include <sivra/core/result.hpp>
#include <sivra/ir/value_type.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace sivra::recovery {

struct type_constraint {
  type_variable_id variable;
  ir::value_type exact_type;
};

struct type_solution {
  type_variable_id variable;
  ir::value_type type;
};

class type_solver {
public:
  type_solver();

  [[nodiscard]] type_variable_id make_variable();
  [[nodiscard]] core::result_t<void> constrain_exact(
    type_variable_id variable,
    ir::value_type type
  );
  [[nodiscard]] core::result_t<std::vector<type_solution>> solve() const;
  [[nodiscard]] std::span<const type_constraint> constraints() const;

private:
  core::owner_token m_owner;
  std::uint32_t m_next_variable = 0;
  std::vector<type_constraint> m_constraints;
};

} // namespace sivra::recovery
