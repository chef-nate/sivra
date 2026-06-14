#include <sivra/recovery/type_solver.hpp>

#include <algorithm>
#include <utility>

namespace sivra::recovery {

type_solver::type_solver() : m_owner(core::owner_token_source::next()) {
}

type_variable_id type_solver::make_variable() {
  return type_variable_id::unsafe_from_index(m_next_variable++, m_owner);
}

core::result_t<void> type_solver::constrain_exact(
  type_variable_id variable,
  ir::value_type type
) {
  if (variable.owner() != m_owner || variable.index() >= m_next_variable) {
    return core::fail<void>(
      "recovery.type_solver.invalid_variable",
      "type constraint references a type_variable_id outside this solver"
    );
  }
  if (auto validated = type.validate(); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }
  m_constraints.push_back({.variable = variable, .exact_type = type});
  return {};
}

core::result_t<std::vector<type_solution>> type_solver::solve() const {
  std::vector<type_solution> solutions;
  for (const auto& constraint : m_constraints) {
    const auto existing = std::ranges::find_if(solutions, [&](const auto& solution) {
      return solution.variable == constraint.variable;
    });
    if (existing == solutions.end()) {
      solutions.push_back({.variable = constraint.variable, .type = constraint.exact_type});
      continue;
    }
    if (existing->type != constraint.exact_type) {
      return core::fail<std::vector<type_solution>>(
        "recovery.type_solver.conflicting_constraints",
        "exact type constraints for one type variable disagree"
      );
    }
  }
  return solutions;
}

std::span<const type_constraint> type_solver::constraints() const {
  return m_constraints;
}

} // namespace sivra::recovery
