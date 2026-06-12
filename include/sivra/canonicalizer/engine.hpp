#pragma once

#include "configuration.hpp"
#include "evaluator.hpp"
#include "phase.hpp"
#include "result.hpp"
#include "rewrite.hpp"

#include <sivra/ir/expression_graph.hpp>

#include <memory>
#include <span>

namespace sivra::canonicalizer {

/**
 * @class engine
 * @brief Applies canonicalization rules to IR expression graphs.
 */
class engine {
public:
  /**
   * @brief Creates an engine with the given canonicalization configuration.
   */
  explicit engine(
    configuration config = {}
  );

  engine(
    configuration config,
    std::shared_ptr<const rule_catalogue> rules,
    std::shared_ptr<const evaluator_catalogue> evaluators
  );

  engine(
    const engine&
  ) = default;

  engine(
    engine&&
  ) = default;

  engine& operator=(
    const engine&
  ) = delete;

  engine& operator=(
    engine&&
  ) = delete;

  /**
   * @brief Returns the configuration used by this engine.
   */
  [[nodiscard]] const configuration& configuration() const;
  [[nodiscard]] const rule_catalogue& rules() const;
  [[nodiscard]] const evaluator_catalogue& evaluators() const;
  [[nodiscard]] const pass_scheduler& scheduler() const;

  /**
   * @brief Canonicalizes one or more root expressions.
   *
   * The returned result contains a fresh graph, with roots preserving the order of
   * the input root span. The result shares the source operation catalogue.
   */
  [[nodiscard]] canonicalization_result canonicalize(
    const ir::expression_graph& graph,
    std::span<const ir::node_id> roots
  ) const;

  /**
   * @brief Canonicalizes one root expression.
   *
   * The result shares the source operation catalogue.
   */
  [[nodiscard]] single_canonicalization_result canonicalize(
    const ir::expression_graph& graph,
    ir::node_id root
  ) const;

private:
  canonicalizer::configuration m_configuration;
  std::shared_ptr<const rule_catalogue> m_rules;
  std::shared_ptr<const evaluator_catalogue> m_evaluators;
  pass_scheduler m_scheduler;
};

} // namespace sivra::canonicalizer
