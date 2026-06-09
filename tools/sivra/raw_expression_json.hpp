#pragma once

#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/id.hpp>
#include <sivra/ir/ir_context.hpp>

#include <memory>
#include <string_view>

namespace sivra::tool {

struct loaded_expression_graph {
  loaded_expression_graph();

  std::unique_ptr<ir::ir_context> context;
  ir::expression_graph graph;
  ir::node_id root;
};

loaded_expression_graph parse_raw_expression_json(
  std::string_view json
);

} // namespace sivra::tool
