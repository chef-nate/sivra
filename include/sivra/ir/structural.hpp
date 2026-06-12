#pragma once

#include "expression_graph.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <map>
#include <vector>

namespace sivra::ir {

struct structural_digest {
  std::array<std::size_t, 2> words;

  auto operator<=>(
    const structural_digest&
  ) const = default;
};

class structural_context {
public:
  structural_digest hash(
    const expression_graph& graph,
    node_id root
  );

  bool equal(
    const expression_graph& lhs_graph,
    node_id lhs,
    const expression_graph& rhs_graph,
    node_id rhs
  );

  std::strong_ordering compare(
    const expression_graph& lhs_graph,
    node_id lhs,
    const expression_graph& rhs_graph,
    node_id rhs
  );

private:
  const std::vector<std::byte>& encoding(
    const expression_graph& graph,
    node_id root
  );

  std::map<const expression_graph*, std::vector<std::vector<std::byte>>> m_encodings;
};

} // namespace sivra::ir
