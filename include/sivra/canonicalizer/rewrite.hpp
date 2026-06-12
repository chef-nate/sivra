#pragma once

#include "configuration.hpp"
#include "evaluator.hpp"
#include "phase.hpp"

#include <sivra/core/diagnostic.hpp>
#include <sivra/core/result.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/graph_builder.hpp>
#include <sivra/ir/structural.hpp>

#include <memory>
#include <span>
#include <string>
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
  ir::operation_id operation;
  std::vector<ir::node_id> operands;
  ir::operation_attributes attributes;
  ir::value_type result_type;
};

struct invalid_rewrite {
  core::diagnostic diagnostic;
};

using rewrite_result = std::variant<no_match, replace_with, rebuild_expression, invalid_rewrite>;

class rewrite_context {
public:
  rewrite_context(
    ir::expression_graph& graph,
    ir::graph_builder& builder,
    const configuration& config,
    ir::structural_context& structural,
    const evaluator_catalogue& evaluators,
    std::size_t& nodes_created
  );

  [[nodiscard]] const ir::expression_graph& graph() const;
  [[nodiscard]] ir::graph_builder& builder() const;
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

  [[nodiscard]] evaluation_result evaluate(
    const rewrite_subject& subject
  ) const;
  [[nodiscard]] evaluation_result evaluate_constants(
    ir::operation_id operation,
    std::span<const ir::constant_value> operands,
    const ir::operation_attributes& attributes,
    ir::value_type result_type
  ) const;
  [[nodiscard]] core::result_t<ir::node_id> make_constant(
    ir::constant_value value
  ) const;
  [[nodiscard]] core::result_t<ir::node_id> rebuild(
    const rebuild_expression& expression
  ) const;

private:
  ir::expression_graph* m_graph;
  ir::graph_builder* m_builder;
  const configuration* m_configuration;
  ir::structural_context* m_structural;
  const evaluator_catalogue* m_evaluators;
  std::size_t* m_nodes_created;
};

using apply_rule_fn = rewrite_result (*)(
  rewrite_context& context,
  const rewrite_subject& subject
);

struct rewrite_rule_metadata {
  rule_id id;
  std::string name;
  pass_phase phase = pass_phase::local_simplification;
  int priority = 0;
  std::string description;
  std::string decreasing_measure;
  bool may_grow = false;
  bool enabled_by_default = true;
};

struct rewrite_rule {
  rewrite_rule_metadata metadata;
  apply_rule_fn apply = nullptr;
};

class rule_catalogue {
public:
  [[nodiscard]] static core::result_t<std::shared_ptr<const rule_catalogue>> create(
    std::vector<rewrite_rule> rules
  );

  [[nodiscard]] const rewrite_rule* find(
    const rule_id& id
  ) const;
  [[nodiscard]] std::span<const rewrite_rule> rules() const;

private:
  explicit rule_catalogue(
    std::vector<rewrite_rule> rules
  );

  std::vector<rewrite_rule> m_rules;
};

[[nodiscard]] std::shared_ptr<const rule_catalogue> builtin_rule_catalogue();

} // namespace sivra::canonicalizer
