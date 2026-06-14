#include "examples.hpp"

#include <raw_expression_json.hpp>

#include <sivra/canonicalizer/engine.hpp>
#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/graph_builder.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/value_type.hpp>
#include <sivra/recovery/recovery.hpp>
#include <sivra/x86/x86.hpp>

#include <CLI/CLI.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <print>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

std::string display_name(
  std::string_view operation
) {
  if (operation == "multiply") {
    return "mul";
  }

  return std::string(operation);
}

std::string format_memory_operand(
  const sivra::compat::raw_memory_operand& value
) {
  auto formatted = std::string("mem[") + value.base_register;
  if (value.offset > 0) {
    formatted += "+" + std::to_string(value.offset);
  } else if (value.offset < 0) {
    formatted += std::to_string(value.offset);
  }
  formatted += "]";
  return formatted;
}

std::string_view format_scalar_category(
  sivra::ir::scalar_category category
) {
  switch (category) {
  case sivra::ir::scalar_category::floating_point:
    return "f";
  case sivra::ir::scalar_category::signed_integer:
    return "i";
  case sivra::ir::scalar_category::unsigned_integer:
    return "u";
  case sivra::ir::scalar_category::unknown:
    return "unknown";
  }
  return "unknown";
}

std::string format_value_type(
  const sivra::ir::value_type& type
) {
  if (type.kind() == sivra::ir::value_type_kind::unknown) {
    return "unknown";
  }
  auto scalar =
    std::string(format_scalar_category(type.category())) + std::to_string(type.element_bit_width());
  if (type.kind() == sivra::ir::value_type_kind::vector) {
    return "vec<" + scalar + ", " + std::to_string(type.lane_count()) + ">";
  }
  return scalar;
}

std::string format_constant(
  const sivra::ir::constant_value& value
) {
  const auto format_element = [](const sivra::ir::scalar_constant_t& element) {
    return std::visit(
      []<typename T>(const T& scalar) { return std::to_string(scalar.value()); }, element
    );
  };

  if (value.result_type().kind() == sivra::ir::value_type_kind::scalar) {
    return format_element(value.element(0));
  }
  const auto type = format_value_type(value.result_type());
  if (value.is_splat()) {
    return type + "(" + format_element(value.element(0)) + ")";
  }
  auto formatted = type + "{";
  for (std::size_t index = 0; index < value.element_count(); ++index) {
    if (index != 0) {
      formatted += ", ";
    }
    formatted += format_element(value.element(index));
  }
  formatted += "}";
  return formatted;
}

std::string indentation(
  std::size_t depth
) {
  return std::string(depth * 2, ' ');
}

bool is_compound_expression(
  const sivra::ir::expression_graph& graph,
  sivra::ir::node_id root
) {
  const auto& node = graph.at(root);
  return node.get_if_operation() != nullptr || node.get_if_merge() != nullptr;
}

std::string format_expression(
  const sivra::ir::expression_graph& graph,
  sivra::ir::node_id root,
  std::span<const sivra::compat::raw_memory_operand> external_values,
  std::size_t depth = 0
) {
  const auto& node = graph.at(root);
  if (const auto* constant = node.get_if_constant()) {
    return format_constant(constant->value);
  }
  if (const auto* symbol = node.get_if_symbol()) {
    return std::string(graph.symbol_name(symbol->symbol));
  }
  if (const auto* external = node.get_if_external_value()) {
    if (external->value.index() < external_values.size()) {
      return format_memory_operand(external_values[external->value.index()]);
    }
    return "external#" + std::to_string(external->value.index());
  }
  if (const auto* unknown = node.get_if_unknown()) {
    return "unknown(" + unknown->reason + ")";
  }
  if (const auto* merge = node.get_if_merge()) {
    const auto needs_block = std::ranges::any_of(merge->incoming, [&](const auto incoming) {
      return is_compound_expression(graph, incoming);
    });
    auto formatted = needs_block ? std::string("merge(\n") : std::string("merge(");
    bool first = true;
    for (const auto incoming : merge->incoming) {
      if (!first) {
        formatted += needs_block ? ",\n" : ", ";
      }
      first = false;
      if (needs_block) {
        formatted += indentation(depth + 1);
      }
      formatted += format_expression(graph, incoming, external_values, depth + 1);
    }
    formatted += needs_block ? "\n" + indentation(depth) + ")" : ")";
    return formatted;
  }

  const auto& application = std::get<sivra::ir::operation_application>(node.payload());
  const auto needs_block = std::ranges::any_of(application.operands, [&](const auto child) {
    return is_compound_expression(graph, child);
  });
  auto formatted = display_name(graph.catalogue().operation(application.operation).name()) +
                   (needs_block ? "(\n" : "(");
  bool first = true;
  for (const auto child : application.operands) {
    if (!first) {
      formatted += needs_block ? ",\n" : ", ";
    }
    first = false;
    if (needs_block) {
      formatted += indentation(depth + 1);
    }
    formatted += format_expression(graph, child, external_values, depth + 1);
  }
  formatted += needs_block ? "\n" + indentation(depth) + ")" : ")";
  return formatted;
}

std::string read_text_file(
  const std::filesystem::path& path
) {
  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("failed to open fixture: " + path.string());
  }
  return {
    std::istreambuf_iterator<char>(stream),
    std::istreambuf_iterator<char>(),
  };
}

std::string diagnostic_message(
  const sivra::core::diagnostic_bundle_t& diagnostics
) {
  if (diagnostics.empty()) {
    return "unknown analysis failure";
  }
  return diagnostics.front().message;
}

std::string_view effect_kind(
  const sivra::program::semantic_effect& effect
) {
  if (std::holds_alternative<sivra::program::semantic_read>(effect)) {
    return "read";
  }
  if (std::holds_alternative<sivra::program::semantic_write>(effect)) {
    return "write";
  }
  if (std::holds_alternative<sivra::program::memory_read_effect>(effect)) {
    return "memory-read";
  }
  if (std::holds_alternative<sivra::program::memory_write_effect>(effect)) {
    return "memory-write";
  }
  if (std::holds_alternative<sivra::program::state_transition_effect>(effect)) {
    return "state";
  }
  return "control";
}

std::uint32_t effect_width(
  const sivra::program::semantic_effect& effect
) {
  if (const auto* read = std::get_if<sivra::program::memory_read_effect>(&effect)) {
    return read->width;
  }
  if (const auto* write = std::get_if<sivra::program::memory_write_effect>(&effect)) {
    return write->width;
  }
  if (const auto* write = std::get_if<sivra::program::semantic_write>(&effect)) {
    if (const auto* destination =
          std::get_if<sivra::program::register_slice>(&write->destination)) {
      return destination->bits.width;
    }
  }
  return 0;
}

void print_assembly_effects(
  std::string_view assembly
) {
  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  auto tokens = tokenizer.tokenize(sivra::core::source_id::from_index(0), assembly);
  if (!tokens.has_value()) {
    throw std::runtime_error(diagnostic_message(tokens.error()));
  }
  auto parsed = parser.parse(*tokens);
  if (!parsed.has_value()) {
    throw std::runtime_error(diagnostic_message(parsed.error()));
  }

  auto instruction_catalogue = sivra::x86::builtin_sse1_instruction_catalogue();
  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(), instruction_catalogue.catalogue
  );
  auto decoded = resolver.resolve(*parsed);
  if (!decoded.has_value()) {
    throw std::runtime_error(diagnostic_message(decoded.error()));
  }

  sivra::x86::semantic_provider provider;
  std::println("decoded {} instruction(s)", decoded->instructions().size());
  for (const auto& instruction : decoded->instructions()) {
    const auto& form = provider.form(instruction.form);
    auto semantics = provider.semantics(instruction);
    if (!semantics.has_value()) {
      throw std::runtime_error(diagnostic_message(semantics.error()));
    }
    std::println("{}: {}", form.mnemonic, form.key);
    for (const auto& effect : semantics->effects) {
      const auto width = effect_width(effect);
      if (width == 0) {
        std::println("  - {}", effect_kind(effect));
      } else {
        std::println("  - {} {}b", effect_kind(effect), width);
      }
    }
    if (semantics->unsupported) {
      std::println("  - unsupported: {}", semantics->unsupported_reason);
    }
  }
}

void print_assembly_recovery(
  std::string_view assembly
) {
  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  auto tokens = tokenizer.tokenize(sivra::core::source_id::from_index(0), assembly);
  if (!tokens.has_value()) {
    throw std::runtime_error(diagnostic_message(tokens.error()));
  }
  auto parsed = parser.parse(*tokens);
  if (!parsed.has_value()) {
    throw std::runtime_error(diagnostic_message(parsed.error()));
  }

  auto instruction_catalogue = sivra::x86::builtin_sse1_instruction_catalogue();
  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(), instruction_catalogue.catalogue
  );
  auto decoded = resolver.resolve(*parsed);
  if (!decoded.has_value()) {
    throw std::runtime_error(diagnostic_message(decoded.error()));
  }
  if (decoded->instructions().empty()) {
    throw std::runtime_error("decoded assembly contains no instructions");
  }

  sivra::x86::semantic_provider provider;
  auto state = sivra::recovery::state_index_builder::build(*decoded, provider);
  if (!state.has_value()) {
    throw std::runtime_error(diagnostic_message(state.error()));
  }

  sivra::ir::operation_catalogue_builder operation_builder;
  auto operation_ids = sivra::ir::register_builtin_operations(operation_builder);
  if (!operation_ids.has_value()) {
    throw std::runtime_error(diagnostic_message(operation_ids.error()));
  }
  auto operations = std::move(operation_builder).freeze();
  if (!operations.has_value()) {
    throw std::runtime_error(diagnostic_message(operations.error()));
  }
  sivra::ir::expression_graph graph(*operations);
  sivra::ir::graph_builder builder(graph);
  sivra::recovery::object_annotation_set annotations;
  sivra::recovery::conservative_memory_alias_analysis alias_analysis;
  sivra::recovery::provenance_store provenance;
  sivra::recovery::recovery_engine engine(
    *decoded, provider, *state, annotations, alias_analysis, graph, builder, provenance
  );

  const auto* xmm0 = provider.registers().find("xmm0");
  if (xmm0 == nullptr) {
    throw std::runtime_error("x86 register catalogue does not contain xmm0");
  }
  const auto& block = decoded->blocks().front();
  const auto point = sivra::program::program_point{
    .block = block.id,
    .instruction = block.instructions.back(),
    .phase = sivra::program::point_phase::after,
  };

  std::println("recovered xmm0:");
  for (std::uint32_t lane = 0; lane < 4; ++lane) {
    const auto query = sivra::recovery::recovery_query{
      .location =
        sivra::program::register_slice{
          .reg = xmm0->definition.id,
          .bits = {.offset = lane * 32U, .width = 32},
          .lane =
            sivra::program::lane_descriptor{
              .index = lane,
              .element_width = 32,
              .lane_count = 4,
            },
        },
      .point = point,
      .expected_type = sivra::ir::value_type::f32(),
    };
    const auto recovered = engine.recover(query);
    if (!recovered.root.has_value()) {
      throw std::runtime_error(diagnostic_message(recovered.diagnostics));
    }
    std::println("  lane {}: {}", lane, format_expression(graph, *recovered.root, {}));
    for (const auto& diag : recovered.diagnostics) {
      std::println("    diagnostic: {}", diag.message);
    }
  }
}

} // namespace

int main(
  int argc,
  char* argv[]
) {
  CLI::App app{"SIVRA"};
  std::string example;
  std::string assembly;
  std::filesystem::path assembly_file;
  bool recover_xmm0 = false;
  std::vector<std::string> example_choices;
  for (const auto name : sivra::tool::example_names()) {
    example_choices.emplace_back(name);
  }

  app.add_option("example", example, "Embedded example")->check(CLI::IsMember(example_choices));
  app.add_option("--asm", assembly, "Textual x86 SSE assembly to decode");
  app.add_option("--asm-file", assembly_file, "Textual x86 SSE assembly file to decode")
    ->check(CLI::ExistingFile);
  app.add_flag("--recover-xmm0", recover_xmm0, "Recover xmm0 lanes for textual x86 SSE input");
  CLI11_PARSE(app, argc, argv);

  try {
    const auto selected_inputs =
      (example.empty() ? 0 : 1) + (assembly.empty() ? 0 : 1) + (assembly_file.empty() ? 0 : 1);
    if (selected_inputs != 1) {
      throw std::runtime_error("provide exactly one embedded example, --asm, or --asm-file");
    }
    if (!assembly.empty() || !assembly_file.empty()) {
      const auto text = !assembly.empty() ? assembly : read_text_file(assembly_file);
      if (recover_xmm0) {
        print_assembly_recovery(text);
      } else {
        print_assembly_effects(text);
      }
      return EXIT_SUCCESS;
    }

    bool first_output = true;
    for (const auto& expression : sivra::tool::example_expressions(example)) {
      const auto path =
        std::filesystem::path(sivra::tool::fixture_directory()) / expression.file_name;
      const auto json = read_text_file(path);
      const auto loaded = sivra::compat::parse_raw_expression_json(json);
      if (!loaded.has_value()) {
        throw std::runtime_error(diagnostic_message(loaded.error()));
      }
      const sivra::canonicalizer::engine canonicalizer;
      const auto canonicalized = canonicalizer.canonicalize(loaded->graph, loaded->root);
      if (!canonicalized.root.has_value()) {
        throw std::runtime_error(diagnostic_message(canonicalized.diagnostics));
      }
      if (!first_output) {
        std::println("");
      }
      first_output = false;
      std::println("{}:", expression.output);
      std::println(
        "{}", format_expression(canonicalized.graph, *canonicalized.root, loaded->external_values)
      );
    }
  } catch (const std::exception& error) {
    std::println(stderr, "error: {}", error.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
