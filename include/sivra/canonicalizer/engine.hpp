#pragma once

#include "configuration.hpp"
#include "result.hpp"

#include <sivra/ir/expression_graph.hpp>

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
};

} // namespace sivra::canonicalizer
