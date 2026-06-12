#pragma once

#include "expression_graph.hpp"

#include <sivra/core/result.hpp>

namespace sivra::ir {

core::result_t<void> validate_graph(
  const expression_graph& graph
);

} // namespace sivra::ir
