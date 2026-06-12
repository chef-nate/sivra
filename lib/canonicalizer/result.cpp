#include <sivra/canonicalizer/result.hpp>

namespace sivra::canonicalizer {

source_mapping::source_mapping(
  core::owner_token source_owner,
  std::size_t source_size
)
    : m_source_owner(source_owner),
      m_nodes(source_size) {
}

void source_mapping::record(
  ir::node_id source,
  ir::node_id canonical
) {
  if (source.owner() != m_source_owner || source.index() >= m_nodes.size()) {
    return;
  }
  m_nodes[source.index()] = canonical;
}

std::optional<ir::node_id> source_mapping::canonical_for(
  ir::node_id source
) const {
  if (source.owner() != m_source_owner || source.index() >= m_nodes.size()) {
    return std::nullopt;
  }
  return m_nodes[source.index()];
}

std::size_t source_mapping::size() const {
  return m_nodes.size();
}

} // namespace sivra::canonicalizer
