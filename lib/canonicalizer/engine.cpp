#include <sivra/canonicalizer/engine.hpp>

#include "termination.hpp"
#include "trace.hpp"
#include "worklist.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void append_diagnostics(
  sivra::core::diagnostic_bundle_t& destination,
  sivra::core::diagnostic_bundle_t source
) {
  destination.insert(
    destination.end(),
    std::make_move_iterator(source.begin()),
    std::make_move_iterator(source.end())
  );
}

void merge_status(
  sivra::core::analysis_status& current,
  sivra::core::analysis_status incoming
) {
  if (incoming == sivra::core::analysis_status::internal_failure ||
      incoming == sivra::core::analysis_status::invalid_input) {
    current = incoming;
  } else if (incoming == sivra::core::analysis_status::resource_exhausted &&
             current == sivra::core::analysis_status::complete) {
    current = incoming;
  }
}

sivra::core::result_t<void> validate_evaluator_bindings(
  const sivra::ir::operation_catalogue& operations,
  const sivra::canonicalizer::evaluator_catalogue& evaluators
) {
  for (const auto& operation : operations.operations()) {
    if (operation.evaluator_key().has_value() &&
        evaluators.find(*operation.evaluator_key()) == nullptr) {
      return sivra::core::fail<void>(
        "canonicalizer.evaluator_catalogue.missing_evaluator",
        "operation declares an evaluator key that is not present in the evaluator catalogue"
      );
    }
  }
  return {};
}

} // namespace

namespace sivra::canonicalizer {

engine::engine(
  canonicalizer::configuration config
)
    : engine(
        std::move(config),
        builtin_rule_catalogue(),
        builtin_evaluator_catalogue()
      ) {
}

engine::engine(
  canonicalizer::configuration config,
  std::shared_ptr<const rule_catalogue> rules,
  std::shared_ptr<const evaluator_catalogue> evaluators
)
    : m_configuration(std::move(config)),
      m_rules(std::move(rules)),
      m_evaluators(std::move(evaluators)),
      m_scheduler(m_rules == nullptr ? *builtin_rule_catalogue() : *m_rules) {
  if (m_rules == nullptr || m_evaluators == nullptr) {
    throw std::invalid_argument("canonicalizer engine requires rule and evaluator catalogues");
  }
}

const configuration& engine::configuration() const {
  return m_configuration;
}

const rule_catalogue& engine::rules() const {
  return *m_rules;
}

const evaluator_catalogue& engine::evaluators() const {
  return *m_evaluators;
}

const pass_scheduler& engine::scheduler() const {
  return m_scheduler;
}

canonicalization_result engine::canonicalize(
  const ir::expression_graph& graph,
  std::span<const ir::node_id> roots
) const {
  termination_tracker termination(m_configuration.limits());
  trace_collector trace(m_configuration.collect_trace());
  core::diagnostic_bundle_t diagnostics;
  core::analysis_status status = core::analysis_status::complete;

  const auto invalid_result = [&](core::diagnostic_bundle_t errors) {
    ir::expression_graph output(graph.shared_catalogue(), graph.external_value_owner());
    return canonicalization_result{
      .graph = std::move(output),
      .roots = {},
      .contract = m_configuration.contract(),
      .status = core::analysis_status::invalid_input,
      .diagnostics = std::move(errors),
      .statistics = termination.statistics(),
      .mapping = source_mapping(graph.owner(), graph.size()),
      .trace = {},
    };
  };

  if (auto validated = m_configuration.validate(); !validated.has_value()) {
    return invalid_result(std::move(validated.error()));
  }
  if (auto validated = m_scheduler.validate(*m_rules); !validated.has_value()) {
    return invalid_result(std::move(validated.error()));
  }
  for (const auto& enabled : m_configuration.enabled_rules()) {
    if (m_rules->find(enabled) == nullptr) {
      return invalid_result(
        core::make_error(
          "canonicalizer.configuration.unknown_rule",
          "enabled rule is not present in the engine rule catalogue: " + std::string(enabled.key())
        )
      );
    }
  }
  if (auto validated = graph.validate(); !validated.has_value()) {
    return invalid_result(std::move(validated.error()));
  }
  if (auto validated = validate_evaluator_bindings(graph.catalogue(), *m_evaluators);
      !validated.has_value()) {
    return invalid_result(std::move(validated.error()));
  }

  worklist_engine worklist(m_configuration, *m_rules, *m_evaluators, termination, trace);
  auto imported = worklist.run(graph, roots, nullptr, true);
  append_diagnostics(diagnostics, std::move(imported.diagnostics));
  merge_status(status, imported.status);
  source_mapping mapping = std::move(imported.mapping);
  auto working_graph = std::move(imported.graph);
  auto working_roots = std::move(imported.roots);

  const auto compose_mapping = [&](source_mapping& current, const source_mapping& next) -> bool {
    auto composed = current.compose(next);
    if (!composed.has_value()) {
      append_diagnostics(diagnostics, std::move(composed.error()));
      status = core::analysis_status::internal_failure;
      return false;
    }
    return true;
  };

  if (status == core::analysis_status::complete) {
    bool stop = false;
    bool changed_sequence = false;
    do {
      changed_sequence = false;
      for (const auto& plan : m_scheduler.plans()) {
        if (stop || plan.rules.empty() || !m_configuration.is_phase_enabled(plan.phase)) {
          continue;
        }

        while (true) {
          if (auto exhausted = termination.begin_phase_iteration()) {
            diagnostics.push_back(std::move(*exhausted));
            status = core::analysis_status::resource_exhausted;
            stop = true;
            break;
          }

          const auto trace_checkpoint = trace.mark();
          auto phase = worklist.run(working_graph, working_roots, &plan, false);
          append_diagnostics(diagnostics, std::move(phase.diagnostics));
          merge_status(status, phase.status);

          if (phase.status != core::analysis_status::complete) {
            if (phase.changed && phase.roots.size() == working_roots.size() &&
                compose_mapping(mapping, phase.mapping)) {
              working_graph = std::move(phase.graph);
              working_roots = std::move(phase.roots);
              changed_sequence = true;
            } else {
              trace.rollback(trace_checkpoint);
            }
            stop = true;
            break;
          }
          if (!phase.changed) {
            trace.rollback(trace_checkpoint);
            break;
          }
          if (phase.roots.size() != working_roots.size() ||
              !compose_mapping(mapping, phase.mapping)) {
            trace.rollback(trace_checkpoint);
            stop = true;
            break;
          }
          changed_sequence = true;
          working_graph = std::move(phase.graph);
          working_roots = std::move(phase.roots);

          if (plan.policy == fixed_point_policy::once) {
            break;
          }
        }
      }
    } while (!stop && changed_sequence);
  }

  if (!working_roots.empty()) {
    auto compacted = worklist.run(working_graph, working_roots, nullptr, false);
    append_diagnostics(diagnostics, std::move(compacted.diagnostics));
    merge_status(status, compacted.status);
    if (compacted.roots.size() == working_roots.size()) {
      if (compose_mapping(mapping, compacted.mapping)) {
        working_graph = std::move(compacted.graph);
        working_roots = std::move(compacted.roots);
      }
    }
  }

  if (auto validated = working_graph.validate(); !validated.has_value()) {
    append_diagnostics(diagnostics, std::move(validated.error()));
    status = core::analysis_status::internal_failure;
  }

  auto statistics = termination.statistics();
  statistics.output_nodes = working_graph.size();
  statistics.peak_output_nodes = std::max(statistics.peak_output_nodes, working_graph.size());
  return {
    .graph = std::move(working_graph),
    .roots = std::move(working_roots),
    .contract = m_configuration.contract(),
    .status = status,
    .diagnostics = std::move(diagnostics),
    .statistics = statistics,
    .mapping = std::move(mapping),
    .trace = trace.take(),
  };
}

single_canonicalization_result engine::canonicalize(
  const ir::expression_graph& graph,
  ir::node_id root
) const {
  const std::array roots{root};
  auto result = canonicalize(graph, std::span<const ir::node_id>(roots));
  const auto rebuilt_root =
    result.roots.empty() ? std::optional<ir::node_id>{} : result.roots.front();
  return {
    .graph = std::move(result.graph),
    .root = rebuilt_root,
    .contract = std::move(result.contract),
    .status = result.status,
    .diagnostics = std::move(result.diagnostics),
    .statistics = result.statistics,
    .mapping = std::move(result.mapping),
    .trace = std::move(result.trace),
  };
}

} // namespace sivra::canonicalizer
