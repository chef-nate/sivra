#pragma once

#include "annotation.hpp"
#include "configuration.hpp"
#include "memory.hpp"
#include "provenance.hpp"
#include "query.hpp"
#include "recovered_object.hpp"
#include "result.hpp"
#include "state_index.hpp"

#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/graph_builder.hpp>
#include <sivra/program/decoded_program.hpp>
#include <sivra/program/semantic_provider.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace sivra::recovery {

struct recovery_statistics {
  std::size_t query_count = 0;
  std::size_t cache_hits = 0;
};

class recovery_engine {
public:
  recovery_engine(
    const program::decoded_program& program,
    const program::semantic_provider& semantics,
    const state_index& state,
    const object_annotation_set& annotations,
    memory_alias_analysis& alias_analysis,
    ir::expression_graph& output_graph,
    ir::graph_builder& output_builder,
    provenance_store& provenance,
    recovery_configuration configuration = {}
  );

  [[nodiscard]] recovery_result recover(
    const recovery_query& query
  );
  [[nodiscard]] std::vector<recovery_result> recover_many(
    std::span<const recovery_query> queries
  );
  [[nodiscard]] core::stage_result<recovered_object_model> build_object_model(
    std::span<const recovery_result> roots
  ) const;
  [[nodiscard]] const recovery_statistics& statistics() const;

private:
  struct cache_entry {
    recovery_query query;
    recovery_result result;
  };

  [[nodiscard]] recovery_result recover_with_cache(
    const recovery_query& query,
    std::size_t depth
  );
  [[nodiscard]] recovery_result recover_uncached(
    const recovery_query& query,
    std::size_t depth
  );
  [[nodiscard]] recovery_result recover_definition(
    const definition_record& definition,
    const recovery_query& query,
    std::size_t depth
  );
  [[nodiscard]] recovery_result recover_predecessor_merge(
    const recovery_query& query,
    std::size_t depth
  );
  [[nodiscard]] recovery_result recover_external_boundary(
    const recovery_query& query,
    std::string detail
  );
  [[nodiscard]] recovery_result lower_lane_expression(
    const definition_record& definition,
    const program::semantic_write& write,
    const program::vector_value& value,
    std::uint32_t lane,
    const recovery_query& query,
    std::size_t depth
  );

  const program::decoded_program* m_program;
  const program::semantic_provider* m_semantics;
  const state_index* m_state;
  const object_annotation_set* m_annotations;
  memory_alias_analysis* m_alias_analysis;
  ir::expression_graph* m_output_graph;
  ir::graph_builder* m_output_builder;
  provenance_store* m_provenance;
  recovery_configuration m_configuration;
  std::vector<cache_entry> m_cache;
  std::vector<recovery_query> m_active_queries;
  recovery_statistics m_statistics;
};

} // namespace sivra::recovery
