#pragma once

#include "options.hpp"
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
   * @brief Creates an engine with the given canonicalization options.
   */
  explicit engine(
    options config = {}
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
   * @brief Returns the options used by this engine.
   */
  [[nodiscard]] const options& config() const;

  /**
   * @brief Canonicalizes one or more root expressions.
   *
   * The returned result contains a fresh graph, with roots preserving the order of
   * the input root span. The result shares the source operation catalogue.
   */
  [[nodiscard]] result canonicalize(
    const ir::expression_graph& graph,
    std::span<const ir::node_id> roots
  ) const;

  /**
   * @brief Canonicalizes one root expression.
   *
   * The result shares the source operation catalogue.
   */
  [[nodiscard]] single_result canonicalize(
    const ir::expression_graph& graph,
    ir::node_id root
  ) const;

private:
  options m_options;
};

} // namespace sivra::canonicalizer
