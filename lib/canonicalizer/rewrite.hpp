#pragma once

#include <sivra/canonicalizer/configuration.hpp>
#include <sivra/core/diagnostic.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/structural.hpp>

#include <variant>
#include <vector>

namespace sivra::canonicalizer {

struct rewrite_subject {
  ir::operation_id operation;
  ir::value_type result_type;
  std::vector<ir::node_id> operands;
  ir::operation_attributes attributes;
};

struct no_match {};

struct replace_with {
  ir::node_id replacement;
};

struct rebuild_expression {
  std::vector<ir::node_id> operands;
};

struct invalid_rewrite {
  core::diagnostic diagnostic;
};

using rewrite_result = std::variant<no_match, replace_with, rebuild_expression, invalid_rewrite>;

class rewrite_context {
public:
  rewrite_context(
    const ir::expression_graph& graph,
    const configuration& config,
    ir::structural_context& structural
  );

  [[nodiscard]] const ir::expression_graph& graph() const;
  [[nodiscard]] const ir::operation_catalogue& catalogue() const;
  [[nodiscard]] ir::structural_context& structural() const;

  [[nodiscard]] const ir::expression_node& node(
    ir::node_id id
  ) const;

  [[nodiscard]] const ir::operation_def& operation(
    ir::operation_id id
  ) const;

  [[nodiscard]] bool is_trait_enabled(
    ir::operation_trait trait
  ) const;

private:
  const ir::expression_graph* m_graph;
  const configuration* m_configuration;
  ir::structural_context* m_structural;
};

} // namespace sivra::canonicalizer
