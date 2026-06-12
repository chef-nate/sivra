#pragma once

#include "expression_graph.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
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
  struct node_record {
    std::vector<std::byte> data;
    std::vector<node_id> children;
    structural_digest digest{};
  };

  struct graph_cache {
    std::vector<node_record> records;
  };

  struct node_pair_key {
    core::owner_token lhs_owner;
    std::uint32_t lhs_index = 0;
    core::owner_token rhs_owner;
    std::uint32_t rhs_index = 0;

    auto operator<=>(
      const node_pair_key&
    ) const = default;
  };

  const node_record& record(
    const expression_graph& graph,
    node_id root
  );

  structural_digest digest_record(
    std::span<const std::byte> data,
    std::span<const node_id> children,
    std::span<const node_record> records
  ) const;

  std::strong_ordering compare_records(
    const expression_graph& lhs_graph,
    node_id lhs,
    const expression_graph& rhs_graph,
    node_id rhs,
    std::map<
      node_pair_key,
      std::strong_ordering
    >& compared
  );

  std::map<core::owner_token, graph_cache> m_graphs;
};

} // namespace sivra::ir
