#include <sivra/canonicalizer/engine.hpp>

#include "rewrite.hpp"
#include "rules/rules.hpp"

#include <sivra/core/budget.hpp>
#include <sivra/ir/graph_builder.hpp>

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

class canonicalization_session {
public:
  canonicalization_session(
    const sivra::ir::expression_graph& source,
    const sivra::canonicalizer::configuration& configuration
  )
      : m_source(source),
        m_configuration(configuration),
        m_output(
          source.shared_catalogue(),
          source.external_value_owner()
        ),
        m_builder(m_output),
        m_mapping(
          source.owner(),
          source.size()
        ),
        m_copied(source.size()),
        m_import_budget(configuration.limits().maximum_imported_nodes),
        m_worklist_budget(configuration.limits().maximum_worklist_steps),
        m_rewrite_budget(configuration.limits().maximum_rewrites) {}

  sivra::canonicalizer::canonicalization_result run(
    std::span<const sivra::ir::node_id> roots
  ) {
    if (auto validated = m_configuration.validate(); !validated.has_value()) {
      append_diagnostics(std::move(validated.error()));
      m_status = sivra::core::analysis_status::invalid_input;
      return finish({});
    }
    if (auto validated = m_source.validate(); !validated.has_value()) {
      append_diagnostics(std::move(validated.error()));
      m_status = sivra::core::analysis_status::invalid_input;
      return finish({});
    }

    std::vector<bool> reachable(m_source.size(), false);
    std::vector<sivra::ir::node_id> pending;
    pending.reserve(roots.size());
    for (const auto root : roots) {
      if (!m_source.contains(root)) {
        add_diagnostic(
          "canonicalizer.invalid_root", "canonicalization root does not belong to the source graph"
        );
        m_status = sivra::core::analysis_status::invalid_input;
        return finish({});
      }
      pending.push_back(root);
    }

    while (!pending.empty()) {
      const auto current = pending.back();
      pending.pop_back();
      if (reachable[current.index()]) {
        continue;
      }
      reachable[current.index()] = true;
      for (const auto operand : m_source.at(current).operands()) {
        pending.push_back(operand);
      }
    }

    for (std::size_t index = 0; index < reachable.size(); ++index) {
      if (!reachable[index]) {
        continue;
      }
      if (!m_import_budget.try_consume()) {
        mark_resource_exhausted(
          "canonicalizer.import_budget", "canonicalizer imported-node budget exhausted"
        );
        break;
      }

      const auto source_id =
        sivra::ir::node_id::unsafe_from_index(static_cast<std::uint32_t>(index), m_source.owner());
      const auto& source_node = m_source.at(source_id);
      std::vector<sivra::ir::node_id> operands;
      operands.reserve(source_node.operands().size());
      bool operands_available = true;
      for (const auto operand : source_node.operands()) {
        if (!m_copied[operand.index()].has_value()) {
          operands_available = false;
          break;
        }
        operands.push_back(*m_copied[operand.index()]);
      }
      if (!operands_available) {
        break;
      }

      auto copied = copy_node(source_node, std::move(operands));
      if (!copied.has_value()) {
        break;
      }
      m_copied[index] = *copied;
      m_mapping.record(source_id, *copied);
      ++m_statistics.imported_nodes;
      trace("import", "source=" + std::to_string(index));
    }

    std::vector<sivra::ir::node_id> output_roots;
    output_roots.reserve(roots.size());
    for (const auto root : roots) {
      if (m_copied[root.index()].has_value()) {
        output_roots.push_back(*m_copied[root.index()]);
      }
    }

    if (auto validated = m_output.validate(); !validated.has_value()) {
      append_diagnostics(std::move(validated.error()));
      m_status = sivra::core::analysis_status::internal_failure;
    }
    return finish(std::move(output_roots));
  }

private:
  std::optional<sivra::ir::node_id> copy_node(
    const sivra::ir::expression_node& source,
    std::vector<sivra::ir::node_id> operands
  ) {
    if (const auto* constant = source.get_if_constant()) {
      return build([&] { return m_builder.make_constant(constant->value); });
    }
    if (const auto* symbol = source.get_if_symbol()) {
      return build([&] {
        return m_builder.make_symbol(
          std::string(m_source.symbol_name(symbol->symbol)), source.result_type()
        );
      });
    }
    if (const auto* external = source.get_if_external_value()) {
      return build([&] {
        return m_builder.make_external_value(external->value, source.result_type());
      });
    }
    if (const auto* unknown = source.get_if_unknown()) {
      return build([&] { return m_builder.make_unknown(unknown->reason, source.result_type()); });
    }
    if (source.get_if_merge() != nullptr) {
      return build([&] { return m_builder.make_merge(operands, source.result_type()); });
    }

    const auto* application = source.get_if_operation();
    if (application == nullptr) {
      add_diagnostic(
        "canonicalizer.invalid_payload", "source node has an unsupported payload kind"
      );
      m_status = sivra::core::analysis_status::internal_failure;
      return std::nullopt;
    }
    return canonicalize_operation(
      application->operation, source.result_type(), std::move(operands), application->attributes
    );
  }

  std::optional<sivra::ir::node_id> canonicalize_operation(
    sivra::ir::operation_id operation,
    sivra::ir::value_type result_type,
    std::vector<sivra::ir::node_id> operands,
    sivra::ir::operation_attributes attributes
  ) {
    sivra::canonicalizer::rewrite_context context(m_output, m_configuration, m_structural);
    sivra::canonicalizer::rewrite_subject subject{
      .operation = operation,
      .result_type = result_type,
      .operands = std::move(operands),
      .attributes = std::move(attributes),
    };

    bool restart = true;
    while (restart && !m_rewrites_disabled) {
      restart = false;
      for (const auto& descriptor : sivra::canonicalizer::scheduled_rules()) {
        if (!m_configuration.is_rule_enabled(descriptor.id)) {
          continue;
        }
        if (!m_worklist_budget.try_consume()) {
          mark_resource_exhausted(
            "canonicalizer.worklist_budget", "canonicalizer worklist budget exhausted"
          );
          m_rewrites_disabled = true;
          break;
        }
        ++m_statistics.worklist_steps;

        auto rewrite = descriptor.apply(context, subject);
        if (std::holds_alternative<sivra::canonicalizer::no_match>(rewrite)) {
          continue;
        }
        if (const auto* invalid = std::get_if<sivra::canonicalizer::invalid_rewrite>(&rewrite)) {
          m_diagnostics.push_back(invalid->diagnostic);
          m_status = sivra::core::analysis_status::internal_failure;
          m_rewrites_disabled = true;
          break;
        }
        if (!m_rewrite_budget.try_consume()) {
          mark_resource_exhausted(
            "canonicalizer.rewrite_budget", "canonicalizer rewrite budget exhausted"
          );
          m_rewrites_disabled = true;
          break;
        }

        ++m_statistics.rewrites_applied;
        trace("rewrite", std::string(descriptor.id.key()));
        if (const auto* replacement = std::get_if<sivra::canonicalizer::replace_with>(&rewrite)) {
          if (!m_output.contains(replacement->replacement) ||
              m_output.at(replacement->replacement).result_type() != subject.result_type) {
            add_diagnostic(
              "canonicalizer.invalid_replacement",
              "rewrite produced a replacement with invalid ownership or type"
            );
            m_status = sivra::core::analysis_status::internal_failure;
            return std::nullopt;
          }
          return replacement->replacement;
        }

        auto rebuilt = std::get<sivra::canonicalizer::rebuild_expression>(std::move(rewrite));
        if (rebuilt.operands == subject.operands) {
          add_diagnostic(
            "canonicalizer.non_progressing_rewrite",
            "rewrite requested an expression rebuild without changing its operands"
          );
          m_status = sivra::core::analysis_status::internal_failure;
          m_rewrites_disabled = true;
          break;
        }
        subject.operands = std::move(rebuilt.operands);
        restart = true;
        break;
      }
    }

    return build([&] {
      return m_builder.apply(
        subject.operation, subject.operands, subject.attributes, subject.result_type
      );
    });
  }

  template <typename Build>
  std::optional<sivra::ir::node_id> build(
    Build&& build_node
  ) {
    if (m_output.size() >= m_configuration.limits().maximum_output_nodes) {
      mark_resource_exhausted(
        "canonicalizer.output_budget", "canonicalizer output-node budget exhausted"
      );
      return std::nullopt;
    }
    auto result = build_node();
    if (!result.has_value()) {
      append_diagnostics(std::move(result.error()));
      m_status = sivra::core::analysis_status::internal_failure;
      return std::nullopt;
    }
    return *result;
  }

  void add_diagnostic(
    std::string code,
    std::string message
  ) {
    m_diagnostics.push_back(
      {
        .code = std::move(code),
        .severity = sivra::core::diagnostic_severity::error,
        .message = std::move(message),
      }
    );
  }

  void append_diagnostics(
    sivra::core::diagnostic_bundle_t diagnostics
  ) {
    m_diagnostics.insert(
      m_diagnostics.end(),
      std::make_move_iterator(diagnostics.begin()),
      std::make_move_iterator(diagnostics.end())
    );
  }

  void mark_resource_exhausted(
    std::string code,
    std::string message
  ) {
    if (!m_resource_diagnostic_emitted) {
      add_diagnostic(std::move(code), std::move(message));
      m_resource_diagnostic_emitted = true;
    }
    if (m_status == sivra::core::analysis_status::complete) {
      m_status = sivra::core::analysis_status::resource_exhausted;
    }
    trace("budget_exhausted", std::string(m_diagnostics.back().code.value()));
  }

  void trace(
    std::string kind,
    std::string detail
  ) {
    if (!m_configuration.collect_trace()) {
      return;
    }
    m_trace.push_back(
      {
        .domain = sivra::core::trace_domain::canonicalizer,
        .kind = std::move(kind),
        .detail = std::move(detail),
      }
    );
  }

  sivra::canonicalizer::canonicalization_result finish(
    std::vector<sivra::ir::node_id> roots
  ) {
    m_statistics.output_nodes = m_output.size();
    return {
      .graph = std::move(m_output),
      .roots = std::move(roots),
      .contract = m_configuration.contract(),
      .status = m_status,
      .diagnostics = std::move(m_diagnostics),
      .statistics = m_statistics,
      .mapping = std::move(m_mapping),
      .trace = std::move(m_trace),
    };
  }

  const sivra::ir::expression_graph& m_source;
  const sivra::canonicalizer::configuration& m_configuration;
  sivra::ir::expression_graph m_output;
  sivra::ir::graph_builder m_builder;
  sivra::ir::structural_context m_structural;
  sivra::canonicalizer::source_mapping m_mapping;
  std::vector<std::optional<sivra::ir::node_id>> m_copied;
  sivra::core::budget_counter m_import_budget;
  sivra::core::budget_counter m_worklist_budget;
  sivra::core::budget_counter m_rewrite_budget;
  sivra::core::analysis_status m_status = sivra::core::analysis_status::complete;
  sivra::core::diagnostic_bundle_t m_diagnostics;
  sivra::canonicalizer::canonicalization_statistics m_statistics;
  std::vector<sivra::core::trace_event> m_trace;
  bool m_rewrites_disabled = false;
  bool m_resource_diagnostic_emitted = false;
};

} // namespace

namespace sivra::canonicalizer {

engine::engine(
  canonicalizer::configuration config
)
    : m_configuration(std::move(config)) {
}

const configuration& engine::configuration() const {
  return m_configuration;
}

canonicalization_result engine::canonicalize(
  const ir::expression_graph& graph,
  std::span<const ir::node_id> roots
) const {
  return canonicalization_session(graph, m_configuration).run(roots);
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
