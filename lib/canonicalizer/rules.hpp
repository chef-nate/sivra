#pragma once

#include <sivra/canonicalizer/rule.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/expression_node.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/operation_registry.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace sivra::canonicalizer {

/**
 * @class rewrite_context
 * @brief Provides canonicalizer rules with controlled access to the rebuilt graph.
 *
 * rewrite_context lets rule implementations inspect copied child nodes, look up
 * operation metadata, and create replacement nodes without depending on engine
 * traversal or source-node memoization details.
 */
class rewrite_context {
public:
  rewrite_context(
    ir::expression_graph& rebuilt
  );

  [[nodiscard]] const ir::operation_def& operation_for(
    const ir::expression_node& source_node
  ) const;

  [[nodiscard]] const ir::expression_node& rebuilt_node(
    ir::node_id node
  ) const;

  [[nodiscard]] ir::node_id copy_node(
    const ir::expression_node& source_node,
    std::vector<ir::node_id> copied_children
  );

private:
  ir::expression_graph& m_rebuilt;
};

enum class rewrite_action {
  unchanged,
  children_changed,
  replaced,
};

struct rewrite_result {
  rewrite_action action = rewrite_action::unchanged;
  std::optional<ir::node_id> replacement;
};

using apply_rule_fn = rewrite_result (*)(
  rewrite_context& context,
  const ir::expression_node& source_node,
  std::vector<ir::node_id>& copied_children
);

/**
 * @struct rule_entry
 * @brief Binds a canonicalizer rule identifier to its implementation function.
 */
struct rule_entry {
  rule id;
  std::string_view name;
  std::string_view description;
  apply_rule_fn apply;
};

/**
 * @brief Returns canonicalizer rules in the order they are applied.
 *
 * The order is generated from rule.def, so adding or reordering rules is done in
 * one place.
 */
[[nodiscard]] std::span<const rule_entry> rule_pipeline();

} // namespace sivra::canonicalizer
