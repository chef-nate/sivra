#include <sivra/canonicalizer/rewrite.hpp>

#include <utility>

namespace sivra::canonicalizer {

rewrite_context::rewrite_context(
  ir::expression_graph& graph,
  ir::graph_builder& builder,
  const configuration& config,
  ir::structural_context& structural,
  const evaluator_catalogue& evaluators,
  std::size_t& nodes_created
)
    : m_graph(&graph),
      m_builder(&builder),
      m_configuration(&config),
      m_structural(&structural),
      m_evaluators(&evaluators),
      m_nodes_created(&nodes_created) {
}

const ir::expression_graph& rewrite_context::graph() const {
  return *m_graph;
}

ir::graph_builder& rewrite_context::builder() const {
  return *m_builder;
}

const ir::operation_catalogue& rewrite_context::catalogue() const {
  return m_graph->catalogue();
}

ir::structural_context& rewrite_context::structural() const {
  return *m_structural;
}

const ir::expression_node& rewrite_context::node(
  ir::node_id id
) const {
  return m_graph->at(id);
}

const ir::operation_def& rewrite_context::operation(
  ir::operation_id id
) const {
  return m_graph->catalogue().operation(id);
}

bool rewrite_context::is_trait_enabled(
  ir::operation_trait trait
) const {
  return m_configuration->is_trait_enabled(trait);
}

evaluation_result rewrite_context::evaluate(
  const rewrite_subject& subject
) const {
  std::vector<ir::constant_value> constants;
  constants.reserve(subject.operands.size());
  for (const auto operand : subject.operands) {
    const auto* constant = node(operand).get_if_constant();
    if (constant == nullptr) {
      return no_evaluation{};
    }
    constants.push_back(constant->value);
  }
  return evaluate_constants(subject.operation, constants, subject.attributes, subject.result_type);
}

evaluation_result rewrite_context::evaluate_constants(
  ir::operation_id operation_id,
  std::span<const ir::constant_value> operands,
  const ir::operation_attributes& attributes,
  ir::value_type result_type
) const {
  const auto& definition = operation(operation_id);
  if (!definition.evaluator_key().has_value()) {
    return no_evaluation{};
  }
  const auto* evaluator = m_evaluators->find(*definition.evaluator_key());
  return evaluator == nullptr ? evaluation_result(no_evaluation{})
                              : evaluator->evaluate(operands, attributes, std::move(result_type));
}

core::result_t<ir::node_id> rewrite_context::make_constant(
  ir::constant_value value
) const {
  auto result = m_builder->make_constant(std::move(value));
  if (result.has_value()) {
    ++*m_nodes_created;
  }
  return result;
}

core::result_t<ir::node_id> rewrite_context::rebuild(
  const rebuild_expression& expression
) const {
  auto result = m_builder->apply(
    expression.operation, expression.operands, expression.attributes, expression.result_type
  );
  if (result.has_value()) {
    ++*m_nodes_created;
  }
  return result;
}

} // namespace sivra::canonicalizer
