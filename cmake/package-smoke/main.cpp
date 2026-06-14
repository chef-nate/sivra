#include <sivra/canonicalizer/canonicalizer.hpp>
#include <sivra/ir/ir.hpp>
#include <sivra/recovery/recovery.hpp>
#include <sivra/x86/x86.hpp>

#include <array>
#include <utility>
#include <variant>

int main() {
  sivra::ir::operation_catalogue_builder catalogue_builder;
  const auto builtins = sivra::ir::register_builtin_operations(catalogue_builder);
  if (!builtins.has_value()) {
    return 1;
  }

  const auto catalogue = std::move(catalogue_builder).freeze();
  if (!catalogue.has_value()) {
    return 2;
  }

  sivra::ir::expression_graph graph(*catalogue);
  sivra::ir::graph_builder graph_builder(graph);
  const auto lhs = graph_builder.make_symbol("lhs", sivra::ir::value_type::f32());
  const auto zero_value = sivra::ir::constant_value::scalar(
    sivra::ir::value_type::f32(), sivra::ir::f32_constant::from_value(0.0F)
  );
  if (!lhs.has_value() || !zero_value.has_value()) {
    return 3;
  }
  const auto zero = graph_builder.make_constant(*zero_value);
  if (!zero.has_value()) {
    return 4;
  }

  const std::array operands{*lhs, *zero};
  const auto root = graph_builder.apply(builtins->add, operands, sivra::ir::value_type::f32());
  if (!root.has_value() || !graph.validate().has_value()) {
    return 5;
  }

  const sivra::canonicalizer::engine canonicalizer;
  const auto result = canonicalizer.canonicalize(graph, *root);
  if (result.status != sivra::core::analysis_status::complete || !result.root.has_value()) {
    return 6;
  }

  if (result.graph.at(*result.root).get_if_symbol() == nullptr) {
    return 7;
  }

  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  const auto tokens =
    tokenizer.tokenize(sivra::core::source_id::from_index(0), "addps xmm0, [rax]");
  if (!tokens.has_value()) {
    return 8;
  }
  const auto parsed = parser.parse(*tokens);
  if (!parsed.has_value()) {
    return 9;
  }
  const auto instructions = sivra::x86::builtin_sse1_instruction_catalogue();
  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(), instructions.catalogue
  );
  const auto decoded = resolver.resolve(*parsed);
  if (!decoded.has_value() || decoded->instructions().empty()) {
    return 10;
  }
  const sivra::x86::semantic_provider provider;
  const auto semantics = provider.semantics(decoded->instructions().front());
  if (!semantics.has_value() || semantics->effects.empty()) {
    return 11;
  }
  const auto* memory_read =
    std::get_if<sivra::program::memory_read_effect>(&semantics->effects.front());
  if (memory_read == nullptr || memory_read->width != 128) {
    return 12;
  }
  const auto state = sivra::recovery::state_index_builder::build(*decoded, provider);
  if (!state.has_value() || state->effects(decoded->instructions().front().id).empty()) {
    return 13;
  }

  return 0;
}
