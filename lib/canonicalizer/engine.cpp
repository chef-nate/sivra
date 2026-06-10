#include <sivra/canonicalizer/engine.hpp>

#include "rules.hpp"

#include <array>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

enum class visit_state {
  unvisited,
  visiting,
  copied,
};

// rebuilds the reachable source slice while preserving source sharing.
class rebuild_context {
public:
  rebuild_context(
    const sivra::ir::expression_graph& source,
    const sivra::canonicalizer::options& config
  )
      : m_source(source),
        m_options(config),
        m_rebuilt(source.shared_catalogue()),
        m_copied(source.size()),
        m_states(
          source.size(),
          visit_state::unvisited
        ) {}

  sivra::ir::node_id copy(
    sivra::ir::node_id source_id
  ) {
    const auto& source_node = m_source.at(source_id);
    const auto index = source_id.index();

    if (m_copied[index].has_value()) {
      return *m_copied[index];
    }

    // recursive rebuild requires a DAG, not a cyclic graph.
    if (m_states[index] == visit_state::visiting) {
      throw std::invalid_argument("expression_graph contains a cycle");
    }

    m_states[index] = visit_state::visiting;

    auto copied_children = copy_children(source_node);

    // store the canonical result for this source node.
    const auto copied_id = canonicalize_node(source_node, std::move(copied_children));

    m_copied[index] = copied_id;
    m_states[index] = visit_state::copied;
    return copied_id;
  }

  sivra::ir::expression_graph take_graph() { return std::move(m_rebuilt); }

private:
  std::vector<sivra::ir::node_id> copy_children(
    const sivra::ir::expression_node& source_node
  ) {
    std::vector<sivra::ir::node_id> copied_children;
    copied_children.reserve(source_node.operands().size());
    for (const auto child : source_node.operands()) {
      copied_children.push_back(copy(child));
    }

    return copied_children;
  }

  sivra::ir::node_id canonicalize_node(
    const sivra::ir::expression_node& source_node,
    std::vector<sivra::ir::node_id> copied_children
  ) {
    sivra::canonicalizer::rewrite_context context(m_rebuilt, m_options);
    if (source_node.get_if_operation() == nullptr) {
      return context.copy_node(source_node, std::move(copied_children));
    }

    for (const auto& entry : sivra::canonicalizer::rule_pipeline()) {
      if (!m_options.is_rule_enabled(entry.id)) {
        continue;
      }

      const auto rewrite = entry.apply(context, source_node, copied_children);
      switch (rewrite.action) {
      case sivra::canonicalizer::rewrite_action::unchanged:
      case sivra::canonicalizer::rewrite_action::children_changed:
        break;

      case sivra::canonicalizer::rewrite_action::replaced:
        if (!rewrite.replacement.has_value()) {
          throw std::logic_error("replacement rewrite has no replacement node");
        }
        return *rewrite.replacement;
      }
    }

    // keep rewrites behind this hook so traversal, cycle checks, and source-node
    // memoization stay independent from canonicalization rules.
    return context.copy_node(source_node, std::move(copied_children));
  }

  const sivra::ir::expression_graph& m_source;
  const sivra::canonicalizer::options& m_options;
  sivra::ir::expression_graph m_rebuilt;
  // m_copied maps each source node to the rebuilt graph node that represents
  // its canonical result.
  std::vector<std::optional<sivra::ir::node_id>> m_copied;
  std::vector<visit_state> m_states;
};

} // namespace

namespace sivra::canonicalizer {

engine::engine(
  options config
)
    : m_options(config) {
}

const options& engine::config() const {
  return m_options;
}

result engine::canonicalize(
  const ir::expression_graph& graph,
  std::span<const ir::node_id> roots
) const {
  rebuild_context rebuild(graph, m_options);

  std::vector<ir::node_id> rebuilt_roots;
  rebuilt_roots.reserve(roots.size());

  for (const auto root : roots) {
    rebuilt_roots.push_back(rebuild.copy(root));
  }

  return result{.graph = rebuild.take_graph(), .roots = std::move(rebuilt_roots)};
}

single_result engine::canonicalize(
  const ir::expression_graph& graph,
  ir::node_id root
) const {
  const std::array roots{root};
  auto rebuilt = canonicalize(graph, std::span<const ir::node_id>(roots));
  return single_result{.graph = std::move(rebuilt.graph), .root = rebuilt.roots.front()};
}

} // namespace sivra::canonicalizer
