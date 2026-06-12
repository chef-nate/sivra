#include <sivra/canonicalizer/result.hpp>

namespace sivra::canonicalizer {

source_mapping::source_mapping(
  core::owner_token source_owner,
  std::size_t source_size
)
    : m_source_owner(source_owner),
      m_nodes(source_size) {
}

core::result_t<void> source_mapping::record(
  ir::node_id source,
  ir::node_id canonical
) {
  if (source.owner() != m_source_owner || source.index() >= m_nodes.size()) {
    return core::fail<void>(
      "canonicalizer.mapping.invalid_source",
      "source mapping record used a node_id outside the source graph"
    );
  }
  m_nodes[source.index()] = canonical;
  return {};
}

std::optional<ir::node_id> source_mapping::canonical_for(
  ir::node_id source
) const {
  if (source.owner() != m_source_owner || source.index() >= m_nodes.size()) {
    return std::nullopt;
  }
  return m_nodes[source.index()];
}

core::result_t<void> source_mapping::compose(
  const source_mapping& next
) {
  for (auto& node : m_nodes) {
    if (node.has_value()) {
      if (node->owner() != next.m_source_owner || node->index() >= next.m_nodes.size()) {
        return core::fail<void>(
          "canonicalizer.mapping.unresolved_compose",
          "source mapping composition used a node_id outside the next source graph"
        );
      }
      node = next.m_nodes[node->index()];
    }
  }
  return {};
}

std::size_t source_mapping::size() const {
  return m_nodes.size();
}

} // namespace sivra::canonicalizer
