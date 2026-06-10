#pragma once

#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/id.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sivra::compat {

struct legacy_memory_ref {
  std::string base_register;
  std::ptrdiff_t offset = 0;
};

struct loaded_expression_graph {
  loaded_expression_graph(
    std::shared_ptr<const ir::operation_catalogue> catalogue,
    ir::expression_graph graph,
    ir::node_id root,
    std::vector<legacy_memory_ref> external_values
  );

  std::shared_ptr<const ir::operation_catalogue> catalogue;
  ir::expression_graph graph;
  ir::node_id root;
  std::vector<legacy_memory_ref> external_values;
};

loaded_expression_graph parse_raw_expression_json(
  std::string_view json
);

} // namespace sivra::compat
