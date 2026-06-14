#include <sivra/recovery/engine.hpp>

#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/operation_catalogue.hpp>
#include <sivra/ir/structural.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace {

using sivra::program::lane_operand_role;
using sivra::program::lane_operation;

sivra::core::diagnostic diagnostic(
  std::string code,
  std::string message
) {
  return {
    .code = std::move(code),
    .severity = sivra::core::diagnostic_severity::error,
    .stage = "recovery",
    .message = std::move(message),
  };
}

sivra::ir::value_type scalar_lane_type(
  const sivra::ir::value_type& type
) {
  if (type.kind() == sivra::ir::value_type_kind::vector) {
    auto lane_type = sivra::ir::value_type::scalar(type.category(), type.element_bit_width());
    if (lane_type.has_value()) {
      return *lane_type;
    }
  }
  return type;
}

std::uint32_t requested_lane(
  const sivra::program::machine_location& location
) {
  const auto* reg = std::get_if<sivra::program::register_slice>(&location);
  if (reg == nullptr) {
    return 0;
  }
  if (reg->lane.has_value()) {
    return reg->lane->index;
  }
  if (reg->bits.width == 32U) {
    return reg->bits.offset / 32U;
  }
  return 0;
}

bool query_matches_definition(
  const sivra::program::semantic_provider& semantics,
  const sivra::program::machine_location& query,
  const sivra::program::machine_location& definition
) {
  const auto relation = semantics.relate(definition, query);
  return relation == sivra::program::location_relation::equal ||
         relation == sivra::program::location_relation::contains;
}

std::optional<std::size_t> instruction_position(
  const sivra::program::basic_block& block,
  sivra::program::instruction_id instruction
) {
  const auto found = std::ranges::find(block.instructions, instruction);
  if (found == block.instructions.end()) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(std::distance(block.instructions.begin(), found));
}

bool definition_reaches_point(
  const sivra::program::decoded_program& program,
  const sivra::recovery::definition_record& definition,
  sivra::program::program_point point
) {
  if (definition.after.block != point.block) {
    return false;
  }
  const auto& block = program.block(point.block);
  const auto definition_position = instruction_position(block, definition.instruction);
  const auto query_position = instruction_position(block, point.instruction);
  if (!definition_position.has_value() || !query_position.has_value()) {
    return false;
  }
  if (*definition_position < *query_position) {
    return true;
  }
  return *definition_position == *query_position &&
         point.phase == sivra::program::point_phase::after;
}

sivra::program::bit_range lane_bits(
  const sivra::program::register_slice& base,
  std::uint32_t lane,
  std::uint32_t element_width
) {
  return {
    .offset = base.bits.offset + lane * element_width,
    .width = element_width,
  };
}

sivra::program::machine_location lane_location(
  const sivra::program::register_slice& base,
  std::uint32_t lane,
  std::uint32_t element_width,
  std::uint32_t lane_count
) {
  return sivra::program::register_slice{
    .reg = base.reg,
    .bits = lane_bits(base, lane, element_width),
    .lane =
      sivra::program::lane_descriptor{
        .index = lane,
        .element_width = element_width,
        .lane_count = lane_count,
      },
  };
}

std::optional<sivra::ir::operation_id> operation_id_for(
  const sivra::ir::operation_catalogue& catalogue,
  std::string_view key
) {
  const auto* operation = catalogue.find(sivra::ir::operation_key(std::string(key)));
  if (operation == nullptr) {
    return std::nullopt;
  }
  return operation->id();
}

std::optional<sivra::ir::operation_id> operation_for_lane_operation(
  const sivra::ir::operation_catalogue& catalogue,
  lane_operation operation
) {
  using enum sivra::program::lane_operation;
  switch (operation) {
  case add_f32:
    return operation_id_for(catalogue, "add");
  case subtract_f32:
    return operation_id_for(catalogue, "subtract");
  case multiply_f32:
    return operation_id_for(catalogue, "multiply");
  case divide_f32:
    return operation_id_for(catalogue, "divide");
  case minimum_f32:
    return operation_id_for(catalogue, "minimum");
  case maximum_f32:
    return operation_id_for(catalogue, "maximum");
  case sqrt_f32:
    return operation_id_for(catalogue, "sqrt");
  case approximate_reciprocal_f32:
    return operation_id_for(catalogue, "reciprocal");
  case approximate_reciprocal_sqrt_f32:
    return operation_id_for(catalogue, "reciprocal_sqrt");
  case bit_and:
    return operation_id_for(catalogue, "bit_and");
  case bit_and_not:
    return operation_id_for(catalogue, "bit_and_not");
  case bit_or:
    return operation_id_for(catalogue, "bit_or");
  case bit_xor:
    return operation_id_for(catalogue, "bit_xor");
  case zero:
  case copy:
    return std::nullopt;
  }
  return std::nullopt;
}

bool memory_transition_reaches_point(
  const sivra::program::decoded_program& program,
  const sivra::recovery::memory_transition& transition,
  sivra::program::program_point point
) {
  if (transition.block != point.block) {
    return false;
  }
  const auto& block = program.block(point.block);
  const auto transition_position = instruction_position(block, transition.instruction);
  const auto query_position = instruction_position(block, point.instruction);
  if (!transition_position.has_value() || !query_position.has_value()) {
    return false;
  }
  if (*transition_position < *query_position) {
    return true;
  }
  return *transition_position == *query_position &&
         point.phase == sivra::program::point_phase::after;
}

bool block_can_reach(
  const sivra::program::decoded_program& program,
  const sivra::recovery::state_index& state,
  sivra::program::basic_block_id from,
  sivra::program::basic_block_id to
) {
  if (from == to) {
    return true;
  }
  if (from.owner() != program.owner() || to.owner() != program.owner() ||
      from.index() >= program.blocks().size() || to.index() >= program.blocks().size()) {
    return false;
  }

  std::vector<bool> visited(program.blocks().size(), false);
  std::vector<sivra::program::basic_block_id> worklist{from};
  visited[from.index()] = true;
  while (!worklist.empty()) {
    const auto current = worklist.back();
    worklist.pop_back();
    for (const auto successor : state.successors(current)) {
      if (successor == to) {
        return true;
      }
      if (successor.owner() != program.owner() || successor.index() >= visited.size() ||
          visited[successor.index()]) {
        continue;
      }
      visited[successor.index()] = true;
      worklist.push_back(successor);
    }
  }
  return false;
}

bool memory_transition_may_reach_point(
  const sivra::program::decoded_program& program,
  const sivra::recovery::state_index& state,
  const sivra::recovery::memory_transition& transition,
  sivra::program::program_point point
) {
  if (transition.block == point.block) {
    return memory_transition_reaches_point(program, transition, point);
  }
  return block_can_reach(program, state, transition.block, point.block);
}

std::optional<sivra::program::program_point> block_exit_point(
  const sivra::program::decoded_program& program,
  sivra::program::basic_block_id block_id
) {
  const auto& block = program.block(block_id);
  if (block.instructions.empty()) {
    return std::nullopt;
  }
  return sivra::program::program_point{
    .block = block.id,
    .instruction = block.instructions.back(),
    .phase = sivra::program::point_phase::after,
  };
}

sivra::core::analysis_status combine_status(
  sivra::core::analysis_status lhs,
  sivra::core::analysis_status rhs
) {
  if (lhs == sivra::core::analysis_status::resource_exhausted ||
      rhs == sivra::core::analysis_status::resource_exhausted) {
    return sivra::core::analysis_status::resource_exhausted;
  }
  if (lhs == sivra::core::analysis_status::internal_failure ||
      rhs == sivra::core::analysis_status::internal_failure) {
    return sivra::core::analysis_status::internal_failure;
  }
  if (lhs == sivra::core::analysis_status::invalid_input ||
      rhs == sivra::core::analysis_status::invalid_input) {
    return sivra::core::analysis_status::invalid_input;
  }
  if (lhs == sivra::core::analysis_status::complete &&
      rhs == sivra::core::analysis_status::complete) {
    return sivra::core::analysis_status::complete;
  }
  return sivra::core::analysis_status::partial;
}

} // namespace

namespace sivra::recovery {

recovery_engine::recovery_engine(
  const program::decoded_program& program,
  const program::semantic_provider& semantics,
  const state_index& state,
  const object_annotation_set& annotations,
  memory_alias_analysis& alias_analysis,
  ir::expression_graph& output_graph,
  ir::graph_builder& output_builder,
  provenance_store& provenance,
  recovery_configuration configuration
)
    : m_program(&program),
      m_semantics(&semantics),
      m_state(&state),
      m_annotations(&annotations),
      m_alias_analysis(&alias_analysis),
      m_output_graph(&output_graph),
      m_output_builder(&output_builder),
      m_provenance(&provenance),
      m_configuration(configuration) {
}

recovery_result recovery_engine::recover(
  const recovery_query& query
) {
  ++m_statistics.query_count;
  return recover_with_cache(query, 0);
}

recovery_result recovery_engine::recover_with_cache(
  const recovery_query& query,
  std::size_t depth
) {
  const auto cached =
    std::ranges::find_if(m_cache, [&](const auto& entry) { return entry.query == query; });
  if (cached != m_cache.end()) {
    ++m_statistics.cache_hits;
    return cached->result;
  }

  const auto active = std::ranges::find(m_active_queries, query);
  if (active != m_active_queries.end()) {
    auto result_type = query.expected_type;
    if (result_type.kind() == ir::value_type_kind::unknown) {
      result_type = ir::value_type::unknown();
    }
    auto root = m_output_builder->make_unknown("cyclic recovery query", result_type);
    recovery_result result{
      .type = result_type,
      .status = core::analysis_status::partial,
      .diagnostics = {diagnostic(
        "recovery.loop.cycle", "recovery query re-entered an active control-flow path"
      )},
    };
    if (root.has_value()) {
      result.root = *root;
      const auto provenance = m_provenance->append(
        provenance_record{
          .kind = provenance_kind::unknown_boundary,
          .point = query.point,
          .location = query.location,
          .node = *root,
          .detail = "cyclic recovery query",
          .diagnostics = result.diagnostics,
        }
      );
      result.provenance = provenance;
    } else {
      result.diagnostics = std::move(root.error());
      result.status = core::analysis_status::invalid_input;
    }
    return result;
  }

  m_active_queries.push_back(query);
  auto result = recover_uncached(query, depth);
  m_active_queries.pop_back();
  m_cache.push_back({.query = query, .result = result});
  return result;
}

std::vector<recovery_result> recovery_engine::recover_many(
  std::span<const recovery_query> queries
) {
  std::vector<recovery_result> results;
  results.reserve(queries.size());
  for (const auto& query : queries) {
    results.push_back(recover(query));
  }
  return results;
}

core::stage_result<recovered_object_model> recovery_engine::build_object_model(
  std::span<const recovery_result> roots
) const {
  recovered_object_builder builder(m_annotations->owner());
  std::vector<recovered_element> elements;
  for (std::size_t index = 0; index < roots.size(); ++index) {
    if (!roots[index].root.has_value() || !roots[index].provenance.has_value()) {
      continue;
    }
    elements.push_back(
      recovered_element{
        .element =
          {
            .object = object_id::unsafe_from_index(0, m_annotations->owner()),
            .indices = {static_cast<std::uint32_t>(index)},
          },
        .root = *roots[index].root,
        .provenance = *roots[index].provenance,
        .complete = roots[index].status == core::analysis_status::complete,
      }
    );
  }
  if (!elements.empty()) {
    const auto object = builder.add_object(std::move(elements));
    (void)object;
  }
  return {
    .artifact = std::move(builder).freeze(),
    .status = core::analysis_status::complete,
  };
}

const recovery_statistics& recovery_engine::statistics() const {
  return m_statistics;
}

recovery_result recovery_engine::recover_uncached(
  const recovery_query& query,
  std::size_t depth
) {
  if (depth > m_configuration.max_recursion_depth) {
    auto root =
      m_output_builder->make_unknown("recovery recursion budget exhausted", query.expected_type);
    recovery_result result{
      .type = query.expected_type,
      .status = core::analysis_status::resource_exhausted,
      .diagnostics = {diagnostic(
        "recovery.query.depth_exhausted", "recovery recursion budget exhausted"
      )},
    };
    if (root.has_value()) {
      result.root = *root;
    }
    return result;
  }

  const definition_record* reaching = nullptr;
  for (const auto& definition : m_state->definitions()) {
    if (!definition_reaches_point(*m_program, definition, query.point)) {
      continue;
    }
    if (!query_matches_definition(*m_semantics, query.location, definition.write.destination)) {
      continue;
    }
    if (reaching == nullptr || definition_reaches_point(*m_program, *reaching, definition.after)) {
      reaching = &definition;
    }
  }

  if (reaching != nullptr) {
    return recover_definition(*reaching, query, depth + 1);
  }

  return recover_predecessor_merge(query, depth + 1);
}

recovery_result recovery_engine::recover_definition(
  const definition_record& definition,
  const recovery_query& query,
  std::size_t depth
) {
  const auto& value = std::get<program::vector_value>(definition.write.value);
  const auto lane = requested_lane(query.location);
  if (lane >= value.lanes.size()) {
    auto root = m_output_builder->make_unknown(
      "requested lane is outside semantic value", query.expected_type
    );
    recovery_result result{
      .type = query.expected_type,
      .status = core::analysis_status::invalid_input,
      .diagnostics = {diagnostic(
        "recovery.query.invalid_lane", "requested lane is outside semantic value"
      )},
    };
    if (root.has_value()) {
      result.root = *root;
    }
    return result;
  }
  return lower_lane_expression(definition, definition.write, value, lane, query, depth + 1);
}

recovery_result recovery_engine::recover_predecessor_merge(
  const recovery_query& query,
  std::size_t depth
) {
  const auto predecessors = m_state->predecessors(query.point.block);
  if (predecessors.empty()) {
    return recover_external_boundary(query, "no reaching definition at function entry");
  }

  std::vector<recovery_result> incoming_results;
  incoming_results.reserve(predecessors.size());
  core::diagnostic_bundle_t diagnostics;
  auto status = core::analysis_status::complete;
  for (const auto predecessor : predecessors) {
    const auto predecessor_exit = block_exit_point(*m_program, predecessor);
    if (!predecessor_exit.has_value()) {
      diagnostics.push_back(diagnostic(
        "recovery.cfg.empty_predecessor",
        "predecessor block has no instruction point that can be queried"
      ));
      status = combine_status(status, core::analysis_status::partial);
      continue;
    }
    auto recovered = recover_with_cache(
      {
        .location = query.location,
        .point = *predecessor_exit,
        .expected_type = query.expected_type,
      },
      depth + 1
    );
    status = combine_status(status, recovered.status);
    diagnostics.insert(
      diagnostics.end(), recovered.diagnostics.begin(), recovered.diagnostics.end()
    );
    incoming_results.push_back(std::move(recovered));
  }

  if (incoming_results.empty()) {
    auto result = recover_external_boundary(query, "no queryable predecessor definitions");
    result.diagnostics.insert(result.diagnostics.end(), diagnostics.begin(), diagnostics.end());
    return result;
  }
  if (incoming_results.size() == 1) {
    auto result = std::move(incoming_results.front());
    result.status = combine_status(result.status, status);
    result.diagnostics = std::move(diagnostics);
    return result;
  }

  std::vector<ir::node_id> incoming_roots;
  std::vector<provenance_id> incoming_provenance;
  incoming_roots.reserve(incoming_results.size());
  for (const auto& incoming : incoming_results) {
    if (!incoming.root.has_value()) {
      auto root = m_output_builder->make_unknown(
        "control-flow predecessor recovery failed", query.expected_type
      );
      recovery_result result{
        .type = query.expected_type,
        .status = core::analysis_status::partial,
        .diagnostics = std::move(diagnostics),
      };
      if (root.has_value()) {
        result.root = *root;
      }
      return result;
    }
    incoming_roots.push_back(*incoming.root);
    if (incoming.provenance.has_value()) {
      incoming_provenance.push_back(*incoming.provenance);
    }
  }

  auto result_type = query.expected_type;
  if (result_type.kind() == ir::value_type_kind::unknown) {
    result_type = m_output_graph->at(incoming_roots.front()).result_type();
  }
  for (const auto root : incoming_roots) {
    if (m_output_graph->at(root).result_type() != result_type) {
      auto unknown = m_output_builder->make_unknown(
        "control-flow predecessor values have incompatible types", result_type
      );
      recovery_result result{
        .type = result_type,
        .status = core::analysis_status::partial,
        .diagnostics = {diagnostic(
          "recovery.cfg.merge_type_mismatch",
          "control-flow predecessor values have incompatible types"
        )},
      };
      result.diagnostics.insert(result.diagnostics.end(), diagnostics.begin(), diagnostics.end());
      if (unknown.has_value()) {
        result.root = *unknown;
      }
      return result;
    }
  }

  ir::structural_context structural;
  const auto all_equal = std::ranges::all_of(incoming_roots, [&](const auto root) {
    return structural.equal(*m_output_graph, incoming_roots.front(), *m_output_graph, root);
  });
  if (all_equal) {
    auto result = std::move(incoming_results.front());
    result.status = combine_status(result.status, status);
    result.diagnostics = std::move(diagnostics);
    return result;
  }

  auto merged = m_output_builder->make_merge(incoming_roots, result_type);
  if (!merged.has_value()) {
    return {
      .type = result_type,
      .status = core::analysis_status::invalid_input,
      .diagnostics = std::move(merged.error()),
    };
  }
  const auto provenance = m_provenance->append(
    provenance_record{
      .kind = provenance_kind::control_flow_merge,
      .point = query.point,
      .location = query.location,
      .node = *merged,
      .inputs = std::move(incoming_provenance),
      .detail = "control-flow predecessor merge",
      .diagnostics = diagnostics,
    }
  );
  return {
    .root = *merged,
    .type = result_type,
    .provenance = provenance,
    .status = status,
    .diagnostics = std::move(diagnostics),
  };
}

recovery_result recovery_engine::recover_external_boundary(
  const recovery_query& query,
  std::string detail
) {
  auto result_type = query.expected_type;
  if (result_type.kind() == ir::value_type_kind::unknown) {
    result_type = ir::value_type::unknown();
  }
  auto root = m_output_builder->make_external_value(result_type);
  recovery_result result{
    .type = result_type,
    .status = core::analysis_status::partial,
  };
  if (!root.has_value()) {
    result.status = core::analysis_status::invalid_input;
    result.diagnostics = std::move(root.error());
    return result;
  }
  result.root = *root;
  result.provenance = m_provenance->append(
    provenance_record{
      .kind = provenance_kind::external_input,
      .point = query.point,
      .location = query.location,
      .node = *root,
      .detail = std::move(detail),
    }
  );
  return result;
}

recovery_result recovery_engine::lower_lane_expression(
  const definition_record& definition,
  const program::semantic_write& write,
  const program::vector_value& value,
  std::uint32_t lane,
  const recovery_query& query,
  std::size_t depth
) {
  const auto result_type = query.expected_type.kind() == ir::value_type_kind::unknown
                             ? scalar_lane_type(value.type)
                             : query.expected_type;
  const auto& expression = value.lanes[lane];
  const auto element_width = value.type.kind() == ir::value_type_kind::vector
                               ? value.type.element_bit_width()
                               : value.type.bit_width();
  const auto lane_count =
    value.type.kind() == ir::value_type_kind::vector ? value.type.lane_count() : 1U;

  const auto recover_input = [&](const program::lane_operand_ref& input) -> recovery_result {
    if (input.role == lane_operand_role::old_destination) {
      const auto* destination = std::get_if<program::register_slice>(&write.destination);
      if (destination == nullptr) {
        auto root =
          m_output_builder->make_unknown("old destination is not a register", result_type);
        recovery_result result{
          .type = result_type,
          .status = core::analysis_status::unsupported,
          .diagnostics = {diagnostic(
            "recovery.lowering.unsupported_destination",
            "old destination recovery requires a register write"
          )},
        };
        if (root.has_value()) {
          result.root = *root;
        }
        return result;
      }
      return recover_with_cache(
        {
          .location = lane_location(*destination, input.lane, element_width, lane_count),
          .point = definition.before,
          .expected_type = result_type,
        },
        depth + 1
      );
    }

    const auto& instruction = m_program->instruction(definition.instruction);
    if (input.operand_index >= instruction.operands.size()) {
      auto root = m_output_builder->make_unknown("semantic source operand is missing", result_type);
      recovery_result result{
        .type = result_type,
        .status = core::analysis_status::invalid_input,
        .diagnostics = {diagnostic(
          "recovery.lowering.missing_operand", "semantic source operand is missing"
        )},
      };
      if (root.has_value()) {
        result.root = *root;
      }
      return result;
    }

    const auto& operand = instruction.operands[input.operand_index];
    if (const auto* reg = std::get_if<program::register_operand>(&operand)) {
      const program::register_slice slice{
        .reg = reg->reg,
        .bits = reg->slice,
        .lane = reg->lane,
      };
      return recover_with_cache(
        {
          .location = lane_location(slice, input.lane, element_width, lane_count),
          .point = definition.before,
          .expected_type = result_type,
        },
        depth + 1
      );
    }
    if (const auto* memory = std::get_if<program::memory_operand>(&operand)) {
      for (const auto& transition : m_state->memory_transitions()) {
        if (!memory_transition_may_reach_point(
              *m_program, *m_state, transition, definition.before
            )) {
          continue;
        }
        const auto relation = m_alias_analysis->relate(transition.address, *memory);
        if (relation == alias_relation::no_alias) {
          continue;
        }
        if (m_configuration.treat_may_alias_store_as_barrier ||
            relation == alias_relation::must_alias) {
          auto root = m_output_builder->make_unknown(
            "memory read may depend on an earlier store", result_type
          );
          recovery_result result{
            .type = result_type,
            .status = core::analysis_status::partial,
            .diagnostics = {diagnostic(
              "recovery.memory.store_barrier",
              "memory read may depend on an earlier store and was not resolved"
            )},
          };
          if (root.has_value()) {
            result.root = *root;
            result.provenance = m_provenance->append(
              provenance_record{
                .kind = provenance_kind::memory_boundary,
                .point = definition.before,
                .node = *root,
                .detail = "memory store barrier",
                .diagnostics = result.diagnostics,
              }
            );
          }
          return result;
        }
      }

      auto root = m_output_builder->make_external_value(result_type);
      recovery_result result{
        .type = result_type,
        .status = core::analysis_status::partial,
      };
      if (!root.has_value()) {
        result.status = core::analysis_status::invalid_input;
        result.diagnostics = std::move(root.error());
        return result;
      }
      result.root = *root;
      result.provenance = m_provenance->append(
        provenance_record{
          .kind = provenance_kind::memory_boundary,
          .point = definition.before,
          .node = *root,
          .detail = "external memory value",
        }
      );
      return result;
    }

    auto root = m_output_builder->make_unknown("unsupported source operand", result_type);
    recovery_result result{
      .type = result_type,
      .status = core::analysis_status::unsupported,
      .diagnostics = {diagnostic(
        "recovery.lowering.unsupported_operand", "source operand is unsupported"
      )},
    };
    if (root.has_value()) {
      result.root = *root;
    }
    return result;
  };

  if (expression.operation == lane_operation::zero) {
    auto constant = result_type.category() == ir::scalar_category::floating_point
                      ? ir::constant_value::scalar(result_type, ir::f32_constant{.bits = 0})
                      : ir::constant_value::scalar(result_type, ir::i32_constant{.bits = 0});
    if (!constant.has_value()) {
      return {
        .type = result_type,
        .status = core::analysis_status::invalid_input,
        .diagnostics = std::move(constant.error()),
      };
    }
    auto root = m_output_builder->make_constant(*constant);
    if (!root.has_value()) {
      return {
        .type = result_type,
        .status = core::analysis_status::invalid_input,
        .diagnostics = std::move(root.error()),
      };
    }
    const auto provenance = m_provenance->append(
      provenance_record{
        .kind = provenance_kind::instruction_write,
        .instruction = definition.instruction,
        .point = definition.after,
        .location = query.location,
        .node = *root,
        .detail = "zero lane expression",
      }
    );
    return {
      .root = *root,
      .type = result_type,
      .provenance = provenance,
    };
  }

  std::vector<ir::node_id> operands;
  std::vector<provenance_id> input_provenance;
  core::diagnostic_bundle_t diagnostics;
  auto status = core::analysis_status::complete;
  for (const auto& input : expression.inputs) {
    auto recovered = recover_input(input);
    if (!recovered.root.has_value()) {
      auto root = m_output_builder->make_unknown("lane input recovery failed", result_type);
      recovery_result result{
        .type = result_type,
        .status = core::analysis_status::partial,
        .diagnostics = recovered.diagnostics,
      };
      if (root.has_value()) {
        result.root = *root;
      }
      return result;
    }
    operands.push_back(*recovered.root);
    if (recovered.provenance.has_value()) {
      input_provenance.push_back(*recovered.provenance);
    }
    diagnostics.insert(
      diagnostics.end(), recovered.diagnostics.begin(), recovered.diagnostics.end()
    );
    if (recovered.status != core::analysis_status::complete) {
      status = core::analysis_status::partial;
    }
  }

  std::optional<ir::node_id> root;
  if (expression.operation == lane_operation::copy) {
    if (operands.empty()) {
      return {
        .type = result_type,
        .status = core::analysis_status::invalid_input,
        .diagnostics = {diagnostic(
          "recovery.lowering.invalid_copy", "copy lane expression must have one input"
        )},
      };
    }
    root = operands.front();
  } else {
    const auto operation =
      operation_for_lane_operation(m_output_graph->catalogue(), expression.operation);
    if (!operation.has_value()) {
      auto unknown = m_output_builder->make_unknown("unsupported lane operation", result_type);
      recovery_result result{
        .type = result_type,
        .status = core::analysis_status::unsupported,
        .diagnostics = {diagnostic(
          "recovery.lowering.unsupported_lane_operation", "lane operation cannot be lowered"
        )},
      };
      if (unknown.has_value()) {
        result.root = *unknown;
      }
      return result;
    }
    auto applied = m_output_builder->apply(*operation, operands, result_type);
    if (!applied.has_value()) {
      return {
        .type = result_type,
        .status = core::analysis_status::invalid_input,
        .diagnostics = std::move(applied.error()),
      };
    }
    root = *applied;
  }

  if (!root.has_value()) {
    return {
      .type = result_type,
      .status = core::analysis_status::internal_failure,
      .diagnostics = {diagnostic(
        "recovery.lowering.missing_root", "lane lowering did not produce a root"
      )},
    };
  }
  const auto provenance = m_provenance->append(
    provenance_record{
      .kind = provenance_kind::instruction_write,
      .instruction = definition.instruction,
      .point = definition.after,
      .location = query.location,
      .node = *root,
      .inputs = std::move(input_provenance),
      .detail = "lowered lane expression",
      .diagnostics = diagnostics,
    }
  );
  return {
    .root = *root,
    .type = result_type,
    .provenance = provenance,
    .status = status,
    .diagnostics = std::move(diagnostics),
  };
}

} // namespace sivra::recovery
