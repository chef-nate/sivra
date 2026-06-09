#pragma once

#include <sivra/ir/expression_graph.hpp>

#include <string>

namespace sivra::test_support {

[[nodiscard]] std::string format_expression(
  const ir::expression_graph& graph,
  ir::node_id root
);

} // namespace sivra::test_support
