#include "worklist.hpp"

#include <sivra/ir/graph_builder.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <utility>
#include <variant>

namespace {

struct subject_state {
  sivra::ir::operation_id operation;
  sivra::ir::value_type result_type;
  sivra::ir::operation_attributes attributes;
  std::vector<sivra::ir::node_id> operands;

  auto operator<=>(
    const subject_state&
  ) const = default;
};

subject_state make_subject_state(
  const sivra::canonicalizer::rewrite_subject& subject
) {
  return {
    .operation = subject.operation,
    .result_type = subject.result_type,
    .attributes = subject.attributes,
    .operands = subject.operands,
  };
}

} // namespace

namespace sivra::canonicalizer {

worklist_engine::worklist_engine(
  const configuration& configuration,
  const rule_catalogue& rules,
  const evaluator_catalogue& evaluators,
  termination_tracker& termination,
  trace_collector& trace
)
    : m_configuration(&configuration),
      m_rules(&rules),
      m_evaluators(&evaluators),
      m_termination(&termination),
      m_trace(&trace) {
}

worklist_result worklist_engine::run(
  const ir::expression_graph& source,
  std::span<const ir::node_id> roots,
  const pass_plan* plan,
  bool count_imports
) {
  ir::expression_graph output(source.shared_catalogue(), source.external_value_owner());
  ir::graph_builder builder(output);
  ir::structural_context structural;
  source_mapping mapping(source.owner(), source.size());
  std::vector<std::optional<ir::node_id>> copied(source.size());
  core::diagnostic_bundle_t diagnostics;
  core::analysis_status status = core::analysis_status::complete;
  bool changed = false;
  bool rewriting_enabled = plan != nullptr;
  bool resource_diagnostic_emitted = false;

  const auto add_diagnostic = [&](core::diagnostic diagnostic, core::analysis_status new_status) {
    diagnostics.push_back(std::move(diagnostic));
    status = new_status;
  };
  const auto exhaust = [&](core::diagnostic diagnostic) {
    if (!resource_diagnostic_emitted) {
      diagnostics.push_back(std::move(diagnostic));
      resource_diagnostic_emitted = true;
    }
    status = core::analysis_status::resource_exhausted;
    rewriting_enabled = false;
  };
  const auto observe_build = [&]() -> bool {
    if (auto exhausted = m_termination->observe_output_nodes(output.size() + 1, source.size())) {
      exhaust(std::move(*exhausted));
      return false;
    }
    return true;
  };
  const auto built_node = [&](core::result_t<ir::node_id> result) -> std::optional<ir::node_id> {
    if (!result.has_value()) {
      for (auto& diagnostic : result.error()) {
        diagnostics.push_back(std::move(diagnostic));
      }
      status = core::analysis_status::internal_failure;
      rewriting_enabled = false;
      return std::nullopt;
    }
    ++m_termination->statistics().nodes_created;
    return *result;
  };

  std::vector<bool> reachable(source.size(), false);
  std::vector<ir::node_id> pending(roots.begin(), roots.end());
  while (!pending.empty()) {
    const auto current = pending.back();
    pending.pop_back();
    if (!source.contains(current)) {
      add_diagnostic(
        {
          .code = "canonicalizer.invalid_root",
          .severity = core::diagnostic_severity::error,
          .message = "canonicalization root does not belong to the source graph",
        },
        core::analysis_status::invalid_input
      );
      return {
        .graph = std::move(output),
        .roots = {},
        .mapping = std::move(mapping),
        .status = status,
        .diagnostics = std::move(diagnostics),
      };
    }
    if (reachable[current.index()]) {
      continue;
    }
    reachable[current.index()] = true;
    for (const auto operand : source.at(current).operands()) {
      pending.push_back(operand);
    }
  }

  for (std::size_t index = 0; index < reachable.size(); ++index) {
    if (!reachable[index]) {
      continue;
    }
    if (count_imports) {
      if (auto exhausted = m_termination->consume_import()) {
        exhaust(std::move(*exhausted));
        break;
      }
    }

    const auto source_id =
      ir::node_id::unsafe_from_index(static_cast<std::uint32_t>(index), source.owner());
    const auto& source_node = source.at(source_id);
    const auto output_size_before_node = output.size();
    std::vector<ir::node_id> operands;
    operands.reserve(source_node.operands().size());
    bool operands_available = true;
    for (const auto operand : source_node.operands()) {
      if (!copied[operand.index()].has_value()) {
        operands_available = false;
        break;
      }
      operands.push_back(*copied[operand.index()]);
    }
    if (!operands_available || !observe_build()) {
      break;
    }

    std::optional<ir::node_id> result;
    if (const auto* constant = source_node.get_if_constant()) {
      result = built_node(builder.make_constant(constant->value));
    } else if (const auto* symbol = source_node.get_if_symbol()) {
      result = built_node(builder.make_symbol(
        std::string(source.symbol_name(symbol->symbol)), source_node.result_type()
      ));
    } else if (const auto* external = source_node.get_if_external_value()) {
      result = built_node(builder.make_external_value(external->value, source_node.result_type()));
    } else if (const auto* unknown = source_node.get_if_unknown()) {
      result = built_node(builder.make_unknown(unknown->reason, source_node.result_type()));
    } else if (source_node.get_if_merge() != nullptr) {
      result = built_node(builder.make_merge(operands, source_node.result_type()));
    } else if (const auto* application = source_node.get_if_operation()) {
      rewrite_subject subject{
        .operation = application->operation,
        .result_type = source_node.result_type(),
        .operands = std::move(operands),
        .attributes = application->attributes,
      };
      rewrite_context context(
        output,
        builder,
        *m_configuration,
        structural,
        *m_evaluators,
        m_termination->statistics().nodes_created
      );
      std::set<subject_state> states{make_subject_state(subject)};
      bool replaced = false;

      while (rewriting_enabled) {
        bool restart = false;
        for (const auto& rule_identifier : plan->rules) {
          if (!m_configuration->is_rule_enabled(rule_identifier)) {
            continue;
          }
          const auto* rule = m_rules->find(rule_identifier);
          if (rule == nullptr) {
            add_diagnostic(
              {
                .code = "canonicalizer.scheduler.missing_rule",
                .severity = core::diagnostic_severity::error,
                .message = "pass plan references a missing rewrite rule",
              },
              core::analysis_status::internal_failure
            );
            rewriting_enabled = false;
            break;
          }
          if (auto exhausted = m_termination->consume_worklist_step()) {
            exhaust(std::move(*exhausted));
            break;
          }
          ++m_termination->statistics().rule_attempts;

          auto rewrite = rule->apply(context, subject);
          if (auto exhausted = m_termination->observe_output_nodes(output.size(), source.size())) {
            exhaust(std::move(*exhausted));
            break;
          }
          if (std::holds_alternative<no_match>(rewrite)) {
            continue;
          }
          ++m_termination->statistics().rule_matches;
          if (const auto* invalid = std::get_if<invalid_rewrite>(&rewrite)) {
            add_diagnostic(invalid->diagnostic, core::analysis_status::internal_failure);
            rewriting_enabled = false;
            break;
          }
          if (const auto* replacement = std::get_if<replace_with>(&rewrite)) {
            if (!output.contains(replacement->replacement) ||
                output.at(replacement->replacement).result_type() != subject.result_type) {
              add_diagnostic(
                {
                  .code = "canonicalizer.invalid_replacement",
                  .severity = core::diagnostic_severity::error,
                  .message = "rewrite produced a replacement with invalid ownership or type",
                },
                core::analysis_status::internal_failure
              );
              rewriting_enabled = false;
              break;
            }
            if (auto exhausted = m_termination->consume_rewrite(source_id)) {
              exhaust(std::move(*exhausted));
              break;
            }
            changed = true;
            m_trace->record(*rule);
            result = replacement->replacement;
            ++m_termination->statistics().nodes_reused;
            replaced = true;
            break;
          }

          auto rebuilt = std::get<rebuild_expression>(std::move(rewrite));
          rewrite_subject next{
            .operation = rebuilt.operation,
            .result_type = std::move(rebuilt.result_type),
            .operands = std::move(rebuilt.operands),
            .attributes = std::move(rebuilt.attributes),
          };
          const auto next_state = make_subject_state(next);
          if (next_state == make_subject_state(subject)) {
            add_diagnostic(
              {
                .code = "canonicalizer.non_progressing_rewrite",
                .severity = core::diagnostic_severity::error,
                .message = "rewrite rebuilt an expression without changing it",
              },
              core::analysis_status::internal_failure
            );
            rewriting_enabled = false;
            break;
          }
          if (!states.insert(next_state).second) {
            exhaust(
              {
                .code = "canonicalizer.oscillation",
                .severity = core::diagnostic_severity::error,
                .message = "rewrite sequence revisited an earlier expression state",
              }
            );
            break;
          }
          if (auto exhausted = m_termination->consume_rewrite(source_id)) {
            exhaust(std::move(*exhausted));
            break;
          }
          changed = true;
          m_trace->record(*rule);
          subject = std::move(next);
          restart = true;
          break;
        }
        if (replaced || !restart) {
          break;
        }
      }

      if (!result.has_value()) {
        if (output.size() != output_size_before_node && !observe_build()) {
          break;
        }
        result = built_node(builder.apply(
          subject.operation, subject.operands, subject.attributes, subject.result_type
        ));
      }
    } else {
      add_diagnostic(
        {
          .code = "canonicalizer.invalid_payload",
          .severity = core::diagnostic_severity::error,
          .message = "source node has an unsupported payload kind",
        },
        core::analysis_status::internal_failure
      );
    }

    if (!result.has_value()) {
      break;
    }
    if (auto recorded = mapping.record(source_id, *result); !recorded.has_value()) {
      for (auto& diagnostic : recorded.error()) {
        diagnostics.push_back(std::move(diagnostic));
      }
      status = core::analysis_status::internal_failure;
      break;
    }
    copied[index] = *result;
  }

  std::vector<ir::node_id> output_roots;
  output_roots.reserve(roots.size());
  for (const auto root : roots) {
    if (copied[root.index()].has_value()) {
      output_roots.push_back(*copied[root.index()]);
    }
  }
  if (auto validated = output.validate(); !validated.has_value()) {
    for (auto& diagnostic : validated.error()) {
      diagnostics.push_back(std::move(diagnostic));
    }
    status = core::analysis_status::internal_failure;
  }

  return {
    .graph = std::move(output),
    .roots = std::move(output_roots),
    .mapping = std::move(mapping),
    .changed = changed,
    .status = status,
    .diagnostics = std::move(diagnostics),
  };
}

} // namespace sivra::canonicalizer
