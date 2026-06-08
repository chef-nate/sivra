#include <sivra/canonicalizer/engine.hpp>

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

// Rebuilds the reachable source slice while preserving source sharing.
class rebuild_context {
public:
  explicit rebuild_context(
    const sivra::ir::expression_graph& source
  )
      : m_source(source),
        m_copied(source.size()),
        m_states(
          source.size(),
          visit_state::unvisited
        ) {}

  sivra::ir::node_id copy(
    sivra::ir::node_id source_id
  ) {
    const auto index = source_id.value();
    if (index >= m_source.size()) {
      throw std::out_of_range("node_id not in expression_graph");
    }

    if (m_copied[index].has_value()) {
      return *m_copied[index];
    }

    // Recursive rebuild requires a DAG, not a cyclic graph.
    if (m_states[index] == visit_state::visiting) {
      throw std::invalid_argument("expression_graph contains a cycle");
    }

    m_states[index] = visit_state::visiting;

    const auto& source_node = m_source.at(source_id);
    // Copy children first so the parent can reference rebuilt node_id values.
    std::vector<sivra::ir::node_id> copied_children;
    copied_children.reserve(source_node.children().size());
    for (const auto child : source_node.children()) {
      copied_children.push_back(copy(child));
    }

    const auto copied_id = m_rebuilt.add_node(
      source_node.operation(),
      source_node.result_type(),
      std::move(copied_children),
      source_node.leaf_value()
    );

    m_copied[index] = copied_id;
    m_states[index] = visit_state::copied;
    return copied_id;
  }

  sivra::ir::expression_graph take_graph() { return std::move(m_rebuilt); }

private:
  const sivra::ir::expression_graph& m_source;
  sivra::ir::expression_graph m_rebuilt;
  std::vector<std::optional<sivra::ir::node_id>> m_copied;
  std::vector<visit_state> m_states;
};

} // namespace

namespace sivra::canonicalizer {

engine::engine(
  const ir::operation_registry& operations,
  options config
)
    : m_operations(operations),
      m_options(config) {
}

const ir::operation_registry& engine::operations() const {
  return m_operations;
}

const options& engine::config() const {
  return m_options;
}

result engine::canonicalize(
  const ir::expression_graph& graph,
  std::span<const ir::node_id> roots
) const {
  rebuild_context rebuild(graph);

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
